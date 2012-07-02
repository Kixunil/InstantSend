#ifndef PLUGINAPI
#define PLUGINAPI

#include <stdint.h>
#include <typeinfo>
#include <malloc.h>

#include "json.h"

typedef struct {
	uint32_t size;
	char data[];
} anyData;

inline anyData *allocData(size_t size) {
	return (anyData *)malloc(sizeof(anyData) + size);
}

class pluginInstance_t {
	public:
		inline pluginInstance_t() {;}
		virtual void reconfigure(jsonComponent_t *config) = 0;
		virtual ~pluginInstance_t();
};

class peer_t : public pluginInstance_t {
	public:
		virtual int sendData(anyData *data) = 0;
		virtual int recvData(anyData *data) = 0;
//		virtual int sentBytes() = 0;				// Returns number of sent bytes.
//		virtual int recdBytes() = 0;				// Returns number of recieved bytes.
//		virtual void disconnect() = 0;				// This function should allways succeed
};

class serverPlugin_t : public pluginInstance_t {
	public:
		virtual peer_t *acceptClient() = 0;				// On success returns instance of client; on error returns NULL
//		virtual void closeAll() = 0;				// It should iterate whole list of clients and disconnect each of them
};

class authenticationPlugin_t : public pluginInstance_t {
	public:
		virtual int onConnect(peer_t *peer) = 0;	// It should authenticate from client side
		virtual int onAccept(peer_t *client) = 0;			// It should authenticate from server side
};

class pluginInstanceCreator_t {
	public:
		virtual peer_t *newClient(jsonComponent_t *config) = 0;
		virtual serverPlugin_t *newServer(jsonComponent_t *config) = 0;
		virtual authenticationPlugin_t *newAuth(jsonComponent_t *config) = 0;
		virtual ~pluginInstanceCreator_t() = 0;
};

pluginInstanceCreator_t &getPluginInstanceCreator(const string &name); // Plugins can use other plugins

#endif
