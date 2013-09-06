#include <windows.h>
#include <winsock.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdexcept>

#include "pluginapi.h"

#define min(a, b) ((a < b)?a:b)

asm (".section .drectve");
asm (".ascii \"-export:getCreator\"");

using namespace InstantSend;
using namespace std;

class i4tPeer : public Peer {
	private:
		SOCKET fd;
		string machineId;
	public:
		inline i4tPeer(int newfd, const string &mId, PluginEnvironment &env) : Peer(env) {
			fd = newfd;
			machineId = mId;
		}

		~i4tPeer() {
			closesocket(fd);
		}

		void reconfigure(jsonComponent_t *config) {
			(void)config;
		}

		int sendData(anyData *data) {
			uint32_t tosend = data->size;
			int sent = htonl(data->size); 
			char *ptr = data->data;
			do errno = 0;
			while(send(fd, (char *)&sent, sizeof(uint32_t), 0) < 0 && (errno == EAGAIN || errno == EINTR));
			while(tosend) {
				sent = send(fd, ptr, tosend, 0);
				if(sent < 0) {
					if(errno == EAGAIN || errno == EINTR) {
						errno = 0;
						continue;
					}
					fprintf(stderr, "Err: %d\n", WSAGetLastError());
					fflush(stderr);
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
			if(recv(fd, (char *)&tmp, sizeof(uint32_t), 0) < (int64_t)sizeof(uint32_t)) {
				fprintf(stderr, "Couldn't receive message size\n");
				fflush(stderr);
				return 0;
			}
			tmp = ntohl(tmp);
			torecv = min((uint32_t)tmp, data->size);
			data->size = tmp;
			while(torecv) {
				tmp = recv(fd, ptr, torecv, 0);
				if(tmp < 0) {
					if(errno == EAGAIN || errno == EINTR) {
						errno = 0;
						continue;
					}
					fprintf(stderr, "Couldn't receive data\n");
					fflush(stderr);
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

class i4tserver : public ServerPlugin {
	private:
		int fd;
	public:
		i4tserver(const jsonComponent_t &config, PluginEnvironment &env) : ServerPlugin(env) {
			struct sockaddr_in srvaddr;
			int backlog;
			srvaddr.sin_family = AF_INET;
			const jsonObj_t &cfg = dynamic_cast<const jsonObj_t &>(config);
			srvaddr.sin_port = htons(dynamic_cast<const jsonInt_t &>(cfg.gie("port")).getVal());
			try {
				srvaddr.sin_addr.s_addr = inet_addr(dynamic_cast<const jsonStr_t &>(cfg.gie("IP")).getVal().c_str());
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

		auto_ptr<Peer> acceptClient() throw() {
			struct sockaddr_in saddr;
			saddr.sin_family = AF_INET;
			int socksize = sizeof(struct sockaddr_in);
			int res = accept(fd, (struct sockaddr *)&saddr, &socksize);
			if(res < 0) return auto_ptr<Peer>();
			return auto_ptr<Peer>(new i4tPeer(res, string(inet_ntoa(saddr.sin_addr)), mEnv));
		}

		void reconfigure(jsonComponent_t *config) { (void)config; }

		~i4tserver() {
			if(fd > -1) closesocket(fd);
		}
};

class i4tCreator : public ConnectionCreator {
	public:
		inline i4tCreator(PluginEnvironment &env) : ConnectionCreator(env) {}

		auto_ptr<Peer> newClient(const jsonComponent_t &config) throw() {
			int fd;
			try {
				const jsonObj_t &cfg = dynamic_cast<const jsonObj_t &>(config);
				struct sockaddr_in dstaddr, srcaddr;
				const jsonStr_t &dstIP = dynamic_cast<const jsonStr_t &>(cfg.gie("destIP"));
				const jsonInt_t &dstPort = dynamic_cast<const jsonInt_t &>(cfg.gie("destPort"));

				// We will skip bind if no value is set
				srcaddr.sin_family = AF_UNSPEC; // temporary initialize
				try {
					srcaddr.sin_addr.s_addr = inet_addr(dynamic_cast<const jsonStr_t &>(cfg.gie("sourceIP")).getVal().c_str());
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
				dstaddr.sin_addr.s_addr = inet_addr(dstIP.getVal().c_str());
				dstaddr.sin_port = htons(dstPort.getVal());

				connect(fd, (struct sockaddr *)&dstaddr, sizeof(struct sockaddr_in));
				if(errno) throw runtime_error(string("connect: ") + strerror(errno));
				return auto_ptr<Peer>(new i4tPeer(fd, dstIP.getVal(), mEnv));
			}
			catch(exception &e) {
				if(fd > -1) closesocket(fd);
				fd = -1;
				mEnv.log(Logger::Error, e.what());
			}
			catch(...) {
				if(fd > -1) closesocket(fd);
				fd = -1;
				mEnv.log(Logger::Error, "Unknown error occured");
			}
			return auto_ptr<Peer>();
	}

	auto_ptr<ServerPlugin> newServer(const jsonComponent_t &config) throw() {
		try {
			return auto_ptr<ServerPlugin>(new i4tserver(config, mEnv));
		}
		catch(exception &e) {
			mEnv.log(Logger::Error, e.what());
		}
		catch(...) {
			mEnv.log(Logger::Error, "Unknown error occured");
		}
		return auto_ptr<ServerPlugin>();
	}
};

extern "C" {
	PluginInstanceCreator *getCreator(PluginEnvironment &env) {
		static i4tCreator creator(env);
		return &creator;
	}
}
