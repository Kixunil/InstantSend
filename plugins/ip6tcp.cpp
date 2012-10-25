#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdexcept>

#include "pluginapi.h"

#define min(a, b) ((a < b)?a:b)

class i6tPeer : public peer_t {
	private:
		int fd;
		string machineId;
	public:
		inline i6tPeer(int newfd, const string &mId) {
			fd = newfd;
			machineId = mId;
		}

		~i6tPeer() {
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

class i6tserver : public serverPlugin_t {
	private:
		int fd;
	public:
		i6tserver(jsonComponent_t *config) {
			struct sockaddr_in6 srvaddr;
			int backlog;
				srvaddr.sin6_family = AF_INET6;
				srvaddr.sin6_scope_id = 0;
				srvaddr.sin6_flowinfo = 0;
				jsonObj_t &cfg = dynamic_cast<jsonObj_t &>(*config);
				srvaddr.sin6_port = htons(dynamic_cast<jsonInt_t &>(cfg.gie("port")).getVal());
				try {
					inet_pton(AF_INET6, dynamic_cast<jsonStr_t &>(cfg.gie("IP")).getVal().c_str(), (void *)&srvaddr.sin6_addr);
				}
				catch(exception &e) {
					memcpy(srvaddr.sin6_addr.s6_addr, &in6addr_any, sizeof(in6addr_any));
				}

				try {
					backlog = dynamic_cast<jsonInt_t &>(cfg.gie("backlog")).getVal();
				}
				catch(exception &e) {
					backlog = 10;
				}

				fd = socket(AF_INET6, SOCK_STREAM, 0);
				if(fd < 0) throw runtime_error(string("socket: ") + strerror(errno));

				if(bind(fd, (struct sockaddr *)&srvaddr, sizeof(struct sockaddr_in6)) < 0) throw runtime_error(string("bind: ") + strerror(errno));

				if(listen(fd, backlog) < 0) throw runtime_error(string("listen: ") + strerror(errno));

		}

		peer_t *acceptClient() {
			struct sockaddr_in6 saddr;
			saddr.sin6_family = AF_INET6;
			socklen_t socksize = sizeof(struct sockaddr_in6);
			int res = accept(fd, (struct sockaddr *)&saddr, &socksize);
			char buf[256];
			if(res < 0) return NULL;
			return new i6tPeer(res, string(inet_ntop(AF_INET6, &saddr.sin6_addr, buf, 256)));
		}

		void reconfigure(jsonComponent_t *config) {
			(void)config;
		}

		~i6tserver() {
			if(fd > -1) close(fd);
		}
};

class i6tCreator : public connectionCreator_t {
	private:
		string lastErr;
	public:
		i6tCreator() {
			lastErr = "No error";
		}
		peer_t *newClient(jsonComponent_t *config) {
			int fd;
			try {
				jsonObj_t &cfg = dynamic_cast<jsonObj_t &>(*config);
				struct sockaddr_in6 dstaddr, srcaddr;
				jsonStr_t &dstIP = dynamic_cast<jsonStr_t &>(cfg.gie("destIP"));
				jsonInt_t &dstPort = dynamic_cast<jsonInt_t &>(cfg.gie("destPort"));

				// We will skip bind if no value is set
				srcaddr.sin6_family = AF_LOCAL; // temporary initialize
				srcaddr.sin6_scope_id = 0;
				srcaddr.sin6_flowinfo = 0;

				try {
					inet_pton(AF_INET6, dynamic_cast<jsonStr_t &>(cfg.gie("sourceIP")).getVal().c_str(), &srcaddr.sin6_addr);
					srcaddr.sin6_family = AF_INET6;
				}
				catch(exception &e) {
					memcpy(srcaddr.sin6_addr.s6_addr, &in6addr_any, sizeof(in6addr_any));
				}

				try {
					srcaddr.sin6_port = htons(dynamic_cast<jsonInt_t &>(cfg.gie("sourcePort")).getVal());
					srcaddr.sin6_family = AF_INET6;
				}
				catch(exception &e) {
					srcaddr.sin6_port = 0;
				}
				fd = socket(AF_INET6, SOCK_STREAM, 0);
				if(fd < 0) throw runtime_error("socket");


				if(srcaddr.sin6_family == AF_INET6 && bind(fd, (struct sockaddr *)&srcaddr, sizeof(struct sockaddr_in6)) < 0) throw runtime_error("bind");

				dstaddr.sin6_family = AF_INET6;
				dstaddr.sin6_scope_id = 0;
				dstaddr.sin6_flowinfo = 0;

				inet_pton(AF_INET6, dstIP.getVal().c_str(), &dstaddr.sin6_addr);
				dstaddr.sin6_port = htons(dstPort.getVal());

				if(connect(fd, (struct sockaddr *)&dstaddr, sizeof(struct sockaddr_in6)) < 0) throw runtime_error("connect");
				return new i6tPeer(fd, dstIP.getVal());
			}
			catch(exception &e) {
				if(fd > -1) close(fd);
				fd = -1;
				lastErr = e.what();
			}
			catch(...) {
				if(fd > -1) close(fd);
				fd = -1;
			}
			return NULL;
	}

	serverPlugin_t *newServer(jsonComponent_t *config) {
		try {
			return new i6tserver(config);
		}
		catch(exception &e) {
			lastErr = e.what();
			return NULL;
		}
		catch(...) {
			lastErr = "Unknown error";
			return NULL;
		}
	}

	authenticationPlugin_t *newAuth(jsonComponent_t *config) {
		(void)config;
		return NULL;
	}

	const char *getErr() {
		return lastErr.c_str();
	}
};

extern "C" {
	pluginInstanceCreator_t *getCreator() {
		static i6tCreator *creator = new i6tCreator();
		return creator;
	}
}
