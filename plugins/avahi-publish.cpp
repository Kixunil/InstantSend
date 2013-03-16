#include <string.h>
#include <stdio.h>
#include <stdexcept>
#include <unistd.h>

#include <avahi-client/client.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/thread-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>

#include "pluginapi.h"

#define D(MSG) fprintf(stderr, MSG "\n"); fflush(stderr);

using namespace std;


class avahiData {
	private:
		unsigned short port;
	public:
		avahiData(unsigned short p) : port(p) {
		}

		inline unsigned short getPort() { return port; }

		~avahiData() {
		}
};

void clientCallback(AvahiClient *c, AvahiClientState state, void * userdata);
void groupCallback(AvahiEntryGroup *group, AvahiEntryGroupState state, void *userdata);

class serverHandler : public eventServer_t {
	private:
		map<serverController_t *, avahiData *> published;
		AvahiThreadedPoll *pollObj;
		AvahiClient *client;
		AvahiEntryGroup *group;
		char *name;
	public:
		serverHandler() : eventServer_t(), pollObj(avahi_threaded_poll_new()), group(NULL) {
			if(!pollObj) throw runtime_error("Failed to create threaded poll object.");

			char buf[HOST_NAME_MAX + 1];
			gethostname(buf, HOST_NAME_MAX);
			buf[HOST_NAME_MAX] = 0;
			name = avahi_strdup(buf);

			int error;
			client = avahi_client_new(avahi_threaded_poll_get(pollObj), (AvahiClientFlags)0, clientCallback, (void *)this, &error);
			if (!client) throw runtime_error(string("Failed to create client: ") + avahi_strerror(error));

			avahi_threaded_poll_start(pollObj);
		}

		~serverHandler() {
			if (pollObj) {
				avahi_threaded_poll_stop(pollObj);
			}
			// avahi_client_free should free group too
#if 0
			if(group) {
				avahi_entry_group_free(group);
			}
#endif
			if (client) {
				avahi_client_free(client);
				client = NULL;
			}
			if (pollObj) {
				avahi_threaded_poll_free(pollObj);
			}
			
			for(map<serverController_t *, avahiData *>::iterator it = published.begin(); it != published.end(); ++it) {
				delete it->second;
			}
			avahi_free(name);
			fprintf(stderr, "avahi-publish freed\n");
		}

		void createServices() {
			if(!group && !(group = avahi_entry_group_new(client, groupCallback, (void *)this))) return;
			if(!group || !published.size()) return;
			avahi_entry_group_reset(group);
			for(map<serverController_t *, avahiData *>::iterator it = published.begin(); it != published.end(); ++it) {
				avahi_entry_group_add_service(group, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, (AvahiPublishFlags)0, name, "_instantsend._tcp", NULL, NULL, it->second->getPort(), NULL);
			}
			int res = avahi_entry_group_commit(group);
			if(res < 0) throw runtime_error(string("Failed to commit entry group: ") + avahi_strerror(res));
		}

		void callback(AvahiClientState state) {
			switch (state) {
				case AVAHI_CLIENT_S_RUNNING:
					createServices();
					break;
				case AVAHI_CLIENT_FAILURE:
					avahi_threaded_poll_quit(pollObj);
					break;
				case AVAHI_CLIENT_S_REGISTERING:
					if (group)
						avahi_entry_group_reset(group);
					break;
			}

		}

		void callback(AvahiEntryGroupState state) {
			switch (state) {
				case AVAHI_ENTRY_GROUP_COLLISION: {
					char *newname = avahi_alternative_service_name(name);
					fprintf(stderr, "Service name collision, renaming service to '%s'\n", newname);
					avahi_free(name);
					name = newname;
					createServices();
				}
				break;

				case AVAHI_ENTRY_GROUP_FAILURE:
					throw runtime_error(string("Entry group failure: ") + avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(group))));
			}
		}

		void onStarted(serverController_t &server) throw() {
			if(server.getPluginName() != "ip4tcp") return;
			auto_ptr<jsonComponent_t> conf(server.getPluginConf().clone());
			avahi_threaded_poll_lock(pollObj);
			if(published.count(&server)) return;
			try {
				unsigned short port = dynamic_cast<const jsonInt_t &>(dynamic_cast<const jsonObj_t &>(server.getPluginConf()).gie("port")).getVal();

				auto_ptr<avahiData> adata(new avahiData(port));

				published.insert(pair<serverController_t *, avahiData *>(&server, adata.get()));
				adata.release();

				createServices();

			}
			catch(exception &e) {
				fprintf(stderr, "Exception: %s\n", e.what());
			}
			catch(...) {
				fprintf(stderr, "Exception\n");
			}
			avahi_threaded_poll_unlock(pollObj);
		}

		void onStopped(serverController_t &server) throw() {
			if(server.getPluginName() != "ip4tcp") return;
			avahi_threaded_poll_lock(pollObj);
			map<serverController_t *, avahiData *>::iterator it = published.find(&server);
			if(it == published.end()) return;

			delete it->second;
			published.erase(it);

			createServices();
			avahi_threaded_poll_unlock(pollObj);

		}

		inline void setClient(AvahiClient *c) {
			if(client == c) return;
			if(client) avahi_client_free(client);
			client = c;
		}
};

void clientCallback(AvahiClient *c, AvahiClientState state, void * userdata) {
	((serverHandler *)userdata)->setClient(c);
	((serverHandler *)userdata)->callback(state);
}

void groupCallback(AvahiEntryGroup *group, AvahiEntryGroupState state, void *userdata) {
	(void)group;
	((serverHandler *)userdata)->callback(state);
}

class ehCreator_t : public eventHandlerCreator_t {
	private:
		serverHandler sh;
//		eventRegister_t *er;
	public:
		ehCreator_t() : eventHandlerCreator_t(), sh() {}
		const char *getErr() {
			return "No error occured";
		}

		void regEvents(eventRegister_t &reg, jsonComponent_t *config) throw() {
			(void)config;
			//er = &reg;
			reg.regServer(sh);
		}

		void unregEvents(eventRegister_t &reg) throw() {
			reg.unregServer(sh);
		}

		~ehCreator_t() {
//			er->unregServer(sh);
		}
};

extern "C" {
	pluginInstanceCreator_t *getCreator(pluginDestrCallback_t &callback) {
		(void)callback;
		static ehCreator_t creator;
		return &creator;
	}
}
