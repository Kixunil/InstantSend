#ifdef DEBUG
//#include <stdio.h>
#endif

#ifdef ENABLE_ERR
#include <openssl/err.h>
#endif
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include <stdexcept>

#include "pluginapi.h"

//#define DEBUG

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

		SSL* newSSL(BIO *bio) {
			try {
				SSL *ssl = SSL_new(mCtx);
				if(!ssl) throw runtime_error("SSL_new failed");
				SSL_set_bio(ssl, bio, bio);
				return ssl;
			}
			catch(...) {
				BIO_free(bio);
				throw;
			}
		}

		~SslContext() {
			SSL_CTX_free(mCtx);

#ifdef ENABLE_ERR
			ERR_free_strings();
#endif
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
#ifdef ENABLE_X509
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
#else
			mMachineId = "UNKNOWN";
#endif
			FD("Peer ID: %s\n", mMachineId.c_str());
		}

		~i4tsPeer() {
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

class i4tserver : public serverPlugin_t {
	private:
		BIO *mBio;
		SslContext &mSslCtx;
	public:
		i4tserver(SslContext &sslCtx, const jsonComponent_t &config, pluginMultiInstanceCreator_t &creator) : mBio(BIO_new(BIO_s_accept())), serverPlugin_t(creator), mSslCtx(sslCtx) {
			try {
			const jsonObj_t &cfg = dynamic_cast<const jsonObj_t &>(config);
			unsigned short port = dynamic_cast<const jsonInt_t &>(cfg.gie("port")).getVal();
			char buf[256];
			try {
				snprintf(buf, 256, "%s:%hu", dynamic_cast<const jsonStr_t &>(cfg.gie("IP")).getVal().c_str(), port);
			}
			catch(exception &e) {
				snprintf(buf, 256, "*:%hu", port);
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

			BIO_set_accept_port(mBio, buf);
			if(BIO_do_accept(mBio) <= 0) {
#ifdef ENABLE_ERR
				ERR_print_errors_fp(stderr);
#endif
				throw runtime_error("BIO_do_accept failed");
			}
			} catch(...) {
				BIO_free(mBio);
			}

		}

		auto_ptr<peer_t> acceptClient() throw() {
			SSL *clientSsl = NULL;
			try {
				int res = -1;
				while(res < 1) {
					if(BIO_do_accept(mBio) <= 0) {
						throw runtime_error("BIO_do_accept failed");
					}
					clientSsl = mSslCtx.newSSL(BIO_pop(mBio));
					D("SSL created");
					if((res = SSL_accept(clientSsl)) < 1 || SSL_get_verify_result(clientSsl) != X509_V_OK) {
#ifdef ENABLE_ERR
						ERR_print_errors_fp(stderr);
#endif
						//FD("SSL error: %d\n", SSL_get_error(clientSsl, res));
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

		void reconfigure(jsonComponent_t *config) { (void)config; }

		~i4tserver() {
			BIO_free(mBio);
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
				const jsonStr_t &dstIP = dynamic_cast<const jsonStr_t &>(cfg.gie("destIP"));
				const jsonInt_t &dstPort = dynamic_cast<const jsonInt_t &>(cfg.gie("destPort"));

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

				BIO *bio = BIO_new(BIO_s_connect());
				BIO_set_conn_hostname(bio, dstIP.getVal().c_str());
				BIO_set_conn_int_port(bio, dstPort.getVal());

				if(BIO_do_connect(bio) <= 0) {
#ifdef ENABLE_ERR
					ERR_print_errors_fp(stderr);
#endif
					BIO_free(bio);
					throw runtime_error("BIO_do_connect failed.");
				}

				SSL *ssl = mCtx.newSSL(bio);
				int ret = SSL_connect(ssl);
				if(ret <= 0) {
#ifdef ENABLE_ERR
					ERR_print_errors_fp(stderr);
#endif
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
