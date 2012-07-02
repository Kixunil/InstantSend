#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "pluginapi.h"

#define min(a, b) ((a < b)?a:b)

extern "C" const char *lastErr = NULL;

class i4tPeer : public peer_t {
	private:
		int fd;
	public:
		inline i4tPeer(int newfd) {
			fd = newfd;
		}

		~i4tPeer() {
			if(fd > -1) close(fd);
		}

		void reconfigure(jsonComponent_t *config) {;}

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

};

class i4tserver : public serverPlugin_t {
	private:
		int fd;
	public:
		i4tserver(jsonComponent_t *config) {
			struct sockaddr_in srvaddr;
			int backlog;
			try {
				srvaddr.sin_family = AF_INET;
				jsonObj_t &cfg = dynamic_cast<jsonObj_t &>(*config);
				srvaddr.sin_port = htons(dynamic_cast<jsonInt_t &>(cfg.gie("port")).getVal());
				try {
					inet_aton(dynamic_cast<jsonStr_t &>(cfg.gie("IP")).getVal().c_str(), &srvaddr.sin_addr);
				}
				catch(exception &e) {
					srvaddr.sin_addr.s_addr = INADDR_ANY;
				}

				try {
					backlog = dynamic_cast<jsonInt_t &>(cfg.gie("backlog")).getVal();
				}
				catch(exception &e) {
					backlog = 10;
				}

				fd = socket(AF_INET, SOCK_STREAM, 0);
				if(fd < 0) throw "socket";

				if(bind(fd, (struct sockaddr *)&srvaddr, sizeof(struct sockaddr_in)) < 0) throw "bind";

				if(listen(fd, backlog) < 0) throw "listen";

			}
			catch(const char *msg) {
				if(fd > -1) close(fd);
				fd = -1;
				throw msg;
			}
		}

		peer_t *acceptClient() {
			int res = accept(fd, NULL, NULL);
			if(res < 0) return NULL;
			return new i4tPeer(res);
		}

		void reconfigure(jsonComponent_t *config) { ; }

		~i4tserver() {
			if(fd > -1) close(fd);
		}
};

class i4tCreator : public pluginInstanceCreator_t {
	public:
		peer_t *newClient(jsonComponent_t *config) {
			int fd;
			try {
				jsonObj_t &cfg = dynamic_cast<jsonObj_t &>(*config);
				struct sockaddr_in dstaddr, srcaddr;
				jsonStr_t &dstIP = dynamic_cast<jsonStr_t &>(cfg.gie("destIP"));
				jsonInt_t &dstPort = dynamic_cast<jsonInt_t &>(cfg.gie("destPort"));

				// We will skip bind if no value is set
				srcaddr.sin_family = AF_LOCAL; // temporary initialize
				try {
					inet_aton(dynamic_cast<jsonStr_t &>(cfg.gie("sourceIP")).getVal().c_str(), &srcaddr.sin_addr);
					srcaddr.sin_family = AF_INET;
				}
				catch(exception &e) {
					srcaddr.sin_addr.s_addr = INADDR_ANY;
				}

				try {
					srcaddr.sin_port = htons(dynamic_cast<jsonInt_t &>(cfg.gie("sourcePort")).getVal());
					srcaddr.sin_addr.s_addr = INADDR_ANY;
				}
				catch(exception &e) {
					srcaddr.sin_port = 0;
				}
				fd = socket(AF_INET, SOCK_STREAM, 0);
				if(fd < 0) throw "socket";


				if(srcaddr.sin_family == AF_INET && bind(fd, (struct sockaddr *)&srcaddr, sizeof(struct sockaddr_in)) < 0) throw "bind";

				dstaddr.sin_family = AF_INET;
				inet_aton(dstIP.getVal().c_str(), &dstaddr.sin_addr);
				dstaddr.sin_port = htons(dstPort.getVal());

				if(connect(fd, (struct sockaddr *)&dstaddr, sizeof(struct sockaddr_in)) < 0) throw "connect";
				return new i4tPeer(fd);
			}
			catch(char const* pch) {
				if(fd > -1) close(fd);
				fd = -1;
				lastErr = pch;
				return NULL;
			}
			catch(...) {
				if(fd > -1) close(fd);
				fd = -1;
			}

	}

	serverPlugin_t *newServer(jsonComponent_t *config) {
		try {
			return new i4tserver(config);
		}
		catch(const char *msg) {
			lastErr = msg;
			return NULL;
		}
	}

	authenticationPlugin_t *newAuth(jsonComponent_t *config) {
		return NULL;
	}
};

pluginInstanceCreator_t *creator = NULL;

extern "C" {
	pluginInstanceCreator_t *getCreator() {
		if(!creator) creator = new i4tCreator();
		return creator;
	}
}
