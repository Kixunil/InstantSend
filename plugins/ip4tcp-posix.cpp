#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>
#include <string.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include <stdexcept>

#include "pluginapi.h"

#define min(a, b) ((a < b)?a:b)

#ifndef MSG_MORE // For non-Linux Unixes
	#define MSG_MORE 0
#endif

#ifndef MSG_NOSIGNAL // For non-Linux Unixes
	#define MSG_NOSIGNAL 0
#endif

#ifdef DEBUG
#define D(MSG) fprintf(stderr, MSG "\n"); fflush(stderr)
#define FD(FORMAT, ...) fprintf(stderr, FORMAT, ##__VA_ARGS__)
#else
#define D(MSG) while(0)
#endif                 

class i4tPeer : public peer_t {
	private:
		int fd;
		string machineId;
	public:
		inline i4tPeer(int newfd, const string &mId, pluginMultiInstanceCreator_t &creator) : peer_t(creator) {
			fd = newfd;
			machineId = mId;
		}

		~i4tPeer() {
			if(fd > -1) close(fd);
		}

		void reconfigure(jsonComponent_t *config) {
			(void)config;
		}

		int sendData(anyData *data) {
			uint32_t tosend = data->size;
			int sent = htonl(data->size); 
			char *ptr = data->data;
			do errno = 0;
			while(send(fd, &sent, sizeof(uint32_t), MSG_MORE | MSG_NOSIGNAL) < 0 && (errno == EAGAIN || errno == EINTR));
			while(tosend) {
				sent = send(fd, ptr, tosend, MSG_NOSIGNAL);
				if(sent < 0) {
					if(errno == EAGAIN || errno == EINTR) {
						errno = 0;
						continue;
					}
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
			//D("Receiving datagram size");
			if(recv(fd, &tmp, sizeof(uint32_t), MSG_WAITALL) < (int64_t)sizeof(uint32_t)) return 0;
			tmp = ntohl(tmp);
			torecv = min((uint32_t)tmp, data->size);
			data->size = tmp;
			while(torecv) {
				tmp = recv(fd, ptr, torecv, MSG_WAITALL);
				if(tmp < 0) {
					if(errno == EAGAIN || errno == EINTR) {
						errno = 0;
						continue;
					}
					return 0;
				}
				torecv -= tmp;
				ptr += tmp;
			}

			return 1;
		}

		string getMachineIdentifier() {
			return machineId;
		}

};

class i4tserver : public serverPlugin_t, public asyncStop_t {
	private:
		int fd, pipefd[2];
	public:
		i4tserver(const jsonComponent_t &config, pluginMultiInstanceCreator_t &creator) : serverPlugin_t(creator) {
			struct sockaddr_in srvaddr;
			int backlog;
			if(pipe(pipefd) < 0) throw runtime_error(string("pipe: ") + strerror(errno));
				srvaddr.sin_family = AF_INET;
				const jsonObj_t &cfg = dynamic_cast<const jsonObj_t &>(config);
				srvaddr.sin_port = htons(dynamic_cast<const jsonInt_t &>(cfg.gie("port")).getVal());
				try {
					inet_aton(dynamic_cast<const jsonStr_t &>(cfg.gie("IP")).getVal().c_str(), &srvaddr.sin_addr);
				}
				catch(exception &e) {
					srvaddr.sin_addr.s_addr = INADDR_ANY;
				}

				try {
					backlog = dynamic_cast<const jsonInt_t &>(cfg.gie("backlog")).getVal();
				}
				catch(exception &e) {
					backlog = 10;
				}

				fd = socket(AF_INET, SOCK_STREAM, 0);
				if(fd < 0) throw runtime_error(string("socket: ") + strerror(errno));

				if(bind(fd, (struct sockaddr *)&srvaddr, sizeof(struct sockaddr_in)) < 0) throw runtime_error(string("bind: ") + strerror(errno));

				if(listen(fd, backlog) < 0) throw runtime_error(string("listen: ") + strerror(errno));

		}

		auto_ptr<peer_t> acceptClient() throw() {
			try {
				struct sockaddr_in saddr;
				saddr.sin_family = AF_INET;
				socklen_t socksize = sizeof(struct sockaddr_in);
				fd_set fds;
				FD_ZERO(&fds);
				FD_SET(pipefd[0], &fds);
				FD_SET(fd, &fds);

				errno = 0;
				while(select((fd > pipefd[0])?fd+1:pipefd[0]+1, &fds, NULL, NULL, NULL) < 0 && errno == EINTR) errno = 0;

				if(errno) throw runtime_error(string("select: ") + strerror(errno));

				if(FD_ISSET(pipefd[0], &fds)) {
					char buf;
					if(read(pipefd[0], &buf, 1) < 0) throw runtime_error(string("accept: ") + strerror(errno));
					throw runtime_error("stop() called");
				}

				int res = accept(fd, (struct sockaddr *)&saddr, &socksize);
				if(res < 0) throw runtime_error(string("accept: ") + strerror(errno));
				return auto_ptr<peer_t>(new i4tPeer(res, string(inet_ntoa(saddr.sin_addr)), getCreator()));
			}
/*			catch(exception &e) {
				return auto_ptr<peer_t>();
			}*/
			catch(...) {
				return auto_ptr<peer_t>();
			}
		}

		bool stop() {
			char buf = 0;
			return write(pipefd[1], &buf, 1) > 0;
		}

		void reconfigure(jsonComponent_t *config) { (void)config; }

		~i4tserver() {
			if(fd > -1) close(fd);
			close(pipefd[0]);
			close(pipefd[1]);
		}
};

class i4tCreator : public connectionCreator_t {
	private:
		string lastErr;
	public:
		i4tCreator(pluginDestrCallback_t &callback) : connectionCreator_t(callback), lastErr("No error") {
		}

		auto_ptr<peer_t> newClient(const jsonComponent_t &config) throw() {
			int fd = -1;
			try {
				const jsonObj_t &cfg = dynamic_cast<const jsonObj_t &>(config);
				struct sockaddr_in dstaddr, srcaddr;
				const jsonStr_t &dstIP = dynamic_cast<const jsonStr_t &>(cfg.gie("destIP"));
				const jsonInt_t &dstPort = dynamic_cast<const jsonInt_t &>(cfg.gie("destPort"));

				// We will skip bind if no value is set
				srcaddr.sin_family = AF_LOCAL; // temporary initialize
				try {
					inet_aton(dynamic_cast<const jsonStr_t &>(cfg.gie("sourceIP")).getVal().c_str(), &srcaddr.sin_addr);
					srcaddr.sin_family = AF_INET;
				}
				catch(exception &e) {
					srcaddr.sin_addr.s_addr = INADDR_ANY;
				}

				try {
					srcaddr.sin_port = htons(dynamic_cast<const jsonInt_t &>(cfg.gie("sourcePort")).getVal());
					srcaddr.sin_addr.s_addr = INADDR_ANY;
				}
				catch(exception &e) {
					srcaddr.sin_port = 0;
				}
				fd = socket(AF_INET, SOCK_STREAM, 0);
				if(fd < 0) throw runtime_error(string("socket: ") + strerror(errno));


				if(srcaddr.sin_family == AF_INET && bind(fd, (struct sockaddr *)&srcaddr, sizeof(struct sockaddr_in)) < 0) throw runtime_error(string("bind: ") + strerror(errno));

				dstaddr.sin_family = AF_INET;
				inet_aton(dstIP.getVal().c_str(), &dstaddr.sin_addr);
				dstaddr.sin_port = htons(dstPort.getVal());

				if(connect(fd, (struct sockaddr *)&dstaddr, sizeof(struct sockaddr_in)) < 0) throw runtime_error(string("connect: ") + strerror(errno));
				return auto_ptr<peer_t>(new i4tPeer(fd, dstIP.getVal(), *this));
			}
			catch(exception &e) {
				if(fd > -1) close(fd);
				fd = -1;
				lastErr = e.what();
				return auto_ptr<peer_t>();
			}
	}

	auto_ptr<serverPlugin_t> newServer(const jsonComponent_t &config) throw() {
		try {
			return auto_ptr<serverPlugin_t>(new i4tserver(config, *this));
		}
		catch(exception &e) {
			lastErr = e.what();
			return auto_ptr<serverPlugin_t>();
		}
	}

	const char *getErr() {
		return lastErr.c_str();
	}
};

extern "C" {
	pluginInstanceCreator_t *getCreator(pluginDestrCallback_t &callback) {
		static i4tCreator creator(callback);
		return &creator;
	}
}
