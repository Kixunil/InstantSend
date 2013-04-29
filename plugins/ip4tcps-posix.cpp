#ifdef DEBUG
#include <stdio.h>
#endif
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <stdexcept>

#include "pluginapi.h"

#ifndef MSG_MORE // For non-Linux Unixes
	#define MSG_MORE 0
#endif

#ifndef MSG_NOSIGNAL // For non-Linux Unixes
	#define MSG_NOSIGNAL 0
#endif

#define DEBUG

#ifdef DEBUG
#define D(MSG) fprintf(stderr, MSG "\n"); fflush(stderr)
#define FD(FORMAT, ...) fprintf(stderr, FORMAT, ##__VA_ARGS__)
#else
#define D(MSG) while(0)
#define FD(FORMAT, ...) while(0)
#endif                 

class Destructible {
	public:
		virtual ~Destructible() {}
};

/*
class sslSession {
	public:
		inline sslSession(SSL *ssl) mSsl(ssl) {}
	private:
		SSL* mSsl;
};
*/

class Pipe : public Destructible {
	public:
		inline Pipe() {
			if(pipe(mPipeFd) < 0) throw runtime_error(string("pipe: ") + strerror(errno));
		}

		inline ssize_t writeChar(char c) const {
			return write(mPipeFd[1], &c, 1);
		}

		inline ssize_t readChar(char &c) const {
			return read(mPipeFd[0], &c, 1);
		}
		
		inline int getReadFd() const throw() {
			return mPipeFd[0];
		}

		~Pipe() {
			close(mPipeFd[0]);
			close(mPipeFd[1]);
		}
	private:
		int mPipeFd[2];
};

class SockAddr : public Destructible {
	public:
		virtual operator const struct sockaddr *() const = 0;
		virtual size_t size() const = 0;
		~SockAddr() {}
};

class SockAddrIn : public SockAddr {
	public:
		inline SockAddrIn() {
			mAddr.sin_family = AF_INET;
			mAddr.sin_addr.s_addr = INADDR_ANY;
			mAddr.sin_port = 0;
		}

		operator const struct sockaddr *() const {
			return (struct sockaddr *)&mAddr;
		}

		size_t size() const {
			return sizeof(mAddr);
		}

		~SockAddrIn() {}

		inline void setPort(short port) {
			mAddr.sin_port = htons(port);
		}

		inline void setAddr(const string &addr) {
			inet_aton(addr.c_str(), &mAddr.sin_addr);
		}

		SockAddrIn(const string &addr, short port) {
			mAddr.sin_family = AF_INET;
			setAddr(addr);
			setPort(port);
		}


	private:
		struct sockaddr_in mAddr;
};

class Socket {
	public:
		Socket(int fd) : mSockFd(fd) {
			FD("Socket(%d)\n", fd);
		}

		Socket(int domain, int type, int protocol = 0) : mSockFd(socket(domain, type, protocol)) {
			FD("socket(%d, %d, %d) = %d\n", domain, type, protocol, mSockFd);
			if(mSockFd < 0) throw runtime_error(string("socket: ") + strerror(errno));
		}

		Socket() : mSockFd(socket(AF_INET, SOCK_STREAM, 0)) {
			FD("socket(%d, %d, %d) = %d\n", AF_INET, SOCK_STREAM, 0, mSockFd);
			if(mSockFd < 0) throw runtime_error(string("socket: ") + strerror(errno));
		}

		~Socket() {
			if(mSockFd > 0) close(mSockFd);
		}
		
		inline void setLocalAddr(const SockAddr &addr) {
			FD("bind(%d, %p, %zu)\n", mSockFd, (const struct sockaddr *)addr, addr.size());
			if(bind(mSockFd, addr, addr.size()) < 0) throw runtime_error(string("bind: ") + strerror(errno));
		}

		inline void setRemoteAddr(const SockAddr &addr) {
			if(connect(mSockFd, addr, addr.size()) < 0) throw runtime_error(string("connect: ") + strerror(errno));
		}

		inline void setListening(int backlog) {
			FD("listen(%d, %d)\n", mSockFd, backlog);
			if(listen(mSockFd, backlog) < 0) throw runtime_error(string("listen: ") + strerror(errno));
		}

		inline int waitForClient() {
			return accept(mSockFd, NULL, NULL);
		}

		int release() {
			int tmp = mSockFd;
			mSockFd = -1;
			return tmp;
		}

		inline operator int() {
			return mSockFd;
		}
	private:
		int mSockFd;
		Socket(const Socket &); // forbid copying
		Socket &operator=(const Socket &);
};

class SslContext {
	public:
		SslContext() {
			SSL_library_init();
			SSL_load_error_strings();

			mCtx = SSL_CTX_new(SSLv23_method());
			if(!mCtx) throw runtime_error("SSL_CTX_new failed");
			D("I4TSSL loaded");
			//SSL_CTX_set_verify(mCtx, SSL_VERIFY_PEER, NULL);
		}

		inline void setVerify(int flags) {
			SSL_CTX_set_verify(mCtx, flags, NULL);
		}

		bool loadChainFile(const string &chainfile) {
			return SSL_CTX_use_certificate_chain_file(mCtx, chainfile.c_str());
		}

		void loadVerifyFile(const string &file) {
			if(!SSL_CTX_load_verify_locations(mCtx, file.c_str(), NULL)) throw runtime_error("loadVerifyFile failed");
		}

		void loadVerifyDirectory(const string &directory) {
			if(!SSL_CTX_load_verify_locations(mCtx, NULL, directory.c_str())) throw runtime_error("loadVerifyDirectory failed");
		}

		void loadKey(const string &keyfile) {
			if(!SSL_CTX_use_PrivateKey_file(mCtx, keyfile.c_str(), SSL_FILETYPE_PEM)) throw runtime_error("loadKey failed");
		}

		bool loadAcceptedCA(const string &cacertfile) {
			STACK_OF(X509_NAME) *cert_names;
			cert_names = SSL_load_client_CA_file(cacertfile.c_str());
			if(!cert_names) return false;
			SSL_CTX_set_client_CA_list(mCtx, cert_names);
			return true;
		}

		SSL* newSSL(Socket &socket) {
			SSL *ssl = SSL_new(mCtx);
			if(!ssl) throw runtime_error("SSL_new failed");
			if(!SSL_set_fd(ssl, socket)) {
				SSL_free(ssl);
				throw runtime_error("SSL_set_fd failed");
			}
			socket.release();
			return ssl;
		}

		~SslContext() {
			SSL_CTX_free(mCtx);

			ERR_free_strings();
		}
	private:
		SSL_CTX *mCtx;
};

/*
class DestructibleContainer : public Destructible {
	public:
		DestructibleContainer(DestructibleContainer &other) {
			for(typeof(container)::iterator it(other.container.begin()); it != other.container.end(); other.container.erase(it++)) container.insert(*it);
		}

		inline void insert(Destructible &item) {
			container.insert(&item);
		}

		inline void erase(Destructible &item) {
			container.erase(&item);
		}

		inline void freeItem(Destructible &item) {
			container.erase(item);
			delete &item;
		}

		void freeAll() {
			for(typeof(container)::iterator it(container.begin()); it != container.end(); container.erase(it++)) delete *it;
		}

		~DestructibleContainer() {
			freeAll();
		}

	private:
		set<Destructible *> container;
};
*/

class i4tsPeer : public peer_t, public Destructible {
	private:
		SSL *mSsl;
		string mMachineId;
	public:
		i4tsPeer(SSL *ssl, pluginMultiInstanceCreator_t &creator) : peer_t(creator), mSsl(ssl) {
			D("Creating i4tsPeer");
			X509 *cert = SSL_get_peer_certificate(ssl);
			if(!cert) {
				mMachineId = "UNKNOWN";
				D("Warning: unknown client!");
				return;
			}
			X509_NAME *subject = X509_get_subject_name(cert);
			X509_NAME_ENTRY *entry;
			int pos;
			ASN1_STRING *entry_str;
			unsigned char *utf;
			
			pos = X509_NAME_get_index_by_NID(subject, NID_commonName, -1);

			if (pos < 0) mMachineId = "UNKNOWN"; else
			if ((entry = X509_NAME_get_entry(subject, pos)) == NULL) mMachineId = "UNKNOWN"; else
			if ((entry_str = X509_NAME_ENTRY_get_data(entry)) == 0) mMachineId = "UNKNOWN"; else

			ASN1_STRING_to_UTF8(&utf, entry_str);
			mMachineId = (char *)utf;
			X509_free(cert);
			FD("Peer ID: %s\n", mMachineId.c_str());
		}

		~i4tsPeer() {
			close(SSL_get_fd(mSsl));
			SSL_free(mSsl);
		}

		void reconfigure(jsonComponent_t *config) {
			(void)config;
		}

		int sendData(anyData *data) {
			FD("Sending %lu bytes of data through SSL connection\n", (unsigned long)data->size);
			uint32_t tosend = data->size;
			uint32_t sent = htonl(data->size); 
			char *ptr = data->data;
			int ret, errcode;
			do {
				ret = SSL_write(mSsl, &sent, sizeof(uint32_t));
				FD("SSL_write() = %d\n", ret);
			} while(ret < 0 && ((errcode= SSL_get_error(mSsl, errcode)) == SSL_ERROR_WANT_READ || errcode == SSL_ERROR_WANT_WRITE));
			if(ret < 0) return 0;
			while(tosend) {
				sent = SSL_write(mSsl, ptr, tosend);
				if(sent <= 0) {
					int err = SSL_get_error(mSsl, sent);
					if(err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
					return 0;
				}
				tosend -= sent;
				ptr += sent;
			}
			return 1;
		}

		int recvData(anyData *data) {
			char *ptr = data->data;
			uint32_t torecv;
			int tmp;
			D("Waiting for data");
			if(SSL_read(mSsl, &tmp, sizeof(uint32_t)) < (int)sizeof(uint32_t)) return 0;
			tmp = ntohl(tmp);
			FD("Receiving %lu bytes of data from SSL connection\n", (unsigned long)tmp);
			torecv = min((uint32_t)tmp, data->size);
			data->size = tmp;
			while(torecv) {
				tmp = SSL_read(mSsl, ptr, torecv);
				if(tmp <= 0) {
					int err = SSL_get_error(mSsl, tmp);
					if(err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) continue;
					return 0;
				}
				torecv -= tmp;
				ptr += tmp;
			}

			return 1;
		}

		string getMachineIdentifier() {
			return mMachineId;
		}

};

class i4tserver : public serverPlugin_t, public asyncStop_t {
	private:
		Socket mSocket;
		Pipe mPipe;
		SslContext &mSslCtx;
	public:
		i4tserver(SslContext &sslCtx, const jsonComponent_t &config, pluginMultiInstanceCreator_t &creator) : mSocket(), serverPlugin_t(creator), mSslCtx(sslCtx) {
			SockAddrIn srvaddr;
			int backlog;
			const jsonObj_t &cfg = dynamic_cast<const jsonObj_t &>(config);
			srvaddr.setPort(dynamic_cast<const jsonInt_t &>(cfg.gie("port")).getVal());
			try {
				srvaddr.setAddr(dynamic_cast<const jsonStr_t &>(cfg.gie("IP")).getVal().c_str());
			}
			catch(exception &e) {
			}

			try {
				backlog = dynamic_cast<const jsonInt_t &>(cfg.gie("backlog")).getVal();
			}
			catch(exception &e) {
				backlog = 10;
			}

			mSslCtx.loadChainFile(dynamic_cast<const jsonStr_t &>(cfg.gie("chainfile")).getVal());
			mSslCtx.loadKey(dynamic_cast<const jsonStr_t &>(cfg.gie("keyfile")).getVal());
			try {
				mSslCtx.loadVerifyDirectory(dynamic_cast<const jsonStr_t &>(cfg.gie("certpath")).getVal());
				mSslCtx.loadAcceptedCA(dynamic_cast<const jsonStr_t &>(cfg.gie("acceptedCA")).getVal());
				mSslCtx.setVerify(SSL_VERIFY_PEER);
			}
			catch(exception &e) {
				FD("Warning: %s\n"
				   "Peer verification disabled!\n", e.what());
				mSslCtx.setVerify(SSL_VERIFY_NONE);
			}

			mSocket.setLocalAddr(srvaddr);
			mSocket.setListening(backlog);
		}

		auto_ptr<peer_t> acceptClient() throw() {
			SSL *clientSsl = NULL;
			try {
				int res = -1;
				while(res < 1) {
					fd_set fds;
					FD_ZERO(&fds);
					FD_SET(mPipe.getReadFd(), &fds);
					FD_SET(mSocket, &fds);

					errno = 0;
					while(select((mSocket > mPipe.getReadFd())?mSocket + 1:mPipe.getReadFd() + 1, &fds, NULL, NULL, NULL) < 0 && errno == EINTR) errno = 0;

					if(errno) throw runtime_error(string("select: ") + strerror(errno));

					if(FD_ISSET(mPipe.getReadFd(), &fds)) {
						char buf;
						if(mPipe.readChar(buf) < 0) throw runtime_error(string("read: ") + strerror(errno));
						throw runtime_error("stop() called");
					}

					int cfd = mSocket.waitForClient();
					if(cfd < 0) throw runtime_error(string("accept: ") + strerror(errno));
					Socket sock(cfd);
					clientSsl = mSslCtx.newSSL(sock);
					D("SSL created");
					if((res = SSL_accept(clientSsl)) < 1 || SSL_get_verify_result(clientSsl) != X509_V_OK) {
						ERR_print_errors_fp(stderr);
						//FD("SSL error: %d\n", SSL_get_error(clientSsl, res));
						close(SSL_get_fd(clientSsl));
						SSL_free(clientSsl);
						D("Error during handshake");
					} else 
						D("Handshake successfull");
				}
				return auto_ptr<peer_t>(new i4tsPeer(clientSsl, getCreator()));
			}
			catch(exception &e) {
				FD("Exception: %s\n", e.what());
				return auto_ptr<peer_t>();
			}
			catch(...) {
				if(clientSsl) SSL_free(clientSsl);
				return auto_ptr<peer_t>();
			}
		}

		bool stop() {
			return mPipe.writeChar(0) > 0;
		}

		void reconfigure(jsonComponent_t *config) { (void)config; }

		~i4tserver() {
		}
};

class i4tsCreator : public connectionCreator_t {
	private:
		string lastErr;
		SslContext mCtx;
	public:
		i4tsCreator(pluginDestrCallback_t &callback) : connectionCreator_t(callback), lastErr("No error") {
		}

		auto_ptr<peer_t> newClient(const jsonComponent_t &config) throw() {
			try {
				const jsonObj_t &cfg = dynamic_cast<const jsonObj_t &>(config);
				SockAddrIn srcAddr;
				const jsonStr_t &dstIP = dynamic_cast<const jsonStr_t &>(cfg.gie("destIP"));
				const jsonInt_t &dstPort = dynamic_cast<const jsonInt_t &>(cfg.gie("destPort"));

				// We will skip bind if no value is set
				bool doBind = false;
				try {
					srcAddr.setAddr(dynamic_cast<const jsonStr_t &>(cfg.gie("sourceIP")).getVal());
					doBind = true;
				}
				catch(exception &e) {
				}

				try {
					srcAddr.setPort(dynamic_cast<const jsonInt_t &>(cfg.gie("sourcePort")).getVal());
					doBind = true;
				}
				catch(exception &e) {
				}

				mCtx.loadChainFile(dynamic_cast<const jsonStr_t &>(cfg.gie("chainfile")).getVal());
				mCtx.loadKey(dynamic_cast<const jsonStr_t &>(cfg.gie("keyfile")).getVal());

				try {
					mCtx.loadVerifyDirectory(dynamic_cast<const jsonStr_t &>(cfg.gie("certpath")).getVal());
					mCtx.setVerify(SSL_VERIFY_PEER);
				}
				catch(exception &e) {
					FD("Warning: %s\n"
					   "Peer verification disabled!\n", e.what());
					mCtx.setVerify(SSL_VERIFY_NONE);
				}

				Socket sock;

				if(doBind) sock.setLocalAddr(srcAddr);
				sock.setRemoteAddr(SockAddrIn(dstIP.getVal(), dstPort.getVal()));

				SSL *ssl = mCtx.newSSL(sock);
				int ret = SSL_connect(ssl);
				if(ret <= 0) {
					ERR_print_errors_fp(stderr);
					throw runtime_error("SSL_connect failed");
				}

				return auto_ptr<peer_t>(new i4tsPeer(ssl, *this));
			}
			catch(exception &e) {
				FD("Exception: %s\n", e.what());
				lastErr = e.what();
				return auto_ptr<peer_t>();
			}
	}

	auto_ptr<serverPlugin_t> newServer(const jsonComponent_t &config) throw() {
		try {
			return auto_ptr<serverPlugin_t>(new i4tserver(mCtx, config, *this));
		}
		catch(exception &e) {
			lastErr = e.what();
#ifdef DEBUG
			FD("Exception: %s\n", lastErr.c_str());
#endif
			return auto_ptr<serverPlugin_t>();
		}
	}

	const char *getErr() {
		return lastErr.c_str();
	}
};

extern "C" {
	pluginInstanceCreator_t *getCreator(pluginDestrCallback_t &callback) {
		try {
			static i4tsCreator creator(callback);
			return &creator;
		}
		catch(exception &e) {
			FD("exception: %s\n",e.what());
			return NULL;
		}
	}
}
