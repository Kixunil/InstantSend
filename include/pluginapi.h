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

class clientPlugin_t : public pluginInstance_t {
	public:
		virtual int connectTarget(jsonComponent_t *config) = 0; 	// Each function returns 1 on success
		virtual int sendData(anyData *data) = 0;
		virtual int recvData(anyData *data) = 0;
//		virtual int sentBytes() = 0;				// Returns number of sent bytes.
//		virtual int recdBytes() = 0;				// Returns number of recieved bytes.
		virtual void disconnect() = 0;				// This function should allways succeed
};

class client_t : public pluginInstance_t {
	public:
		virtual int sendData(anyData *data) = 0;			// Each function returns 1 on success
		virtual int recvData(anyData *data) = 0;
//		virtual int sentBytes() = 0;				// Returns number of sent bytes.
//		virtual int recdBytes() = 0;				// Returns number of recieved bytes.
		virtual void disconnect() = 0;				// This function should allways succeed

};

class serverPlugin_t : public pluginInstance_t {
	public:
		virtual client_t *acceptClient() = 0;				// On success returns instance of client; on error returns NULL
//		virtual void closeAll() = 0;				// It should iterate whole list of clients and disconnect each of them
};

class authenticationPlugin_t : public pluginInstance_t {
	public:
		virtual int onConnect(clientPlugin_t *clientPlugin) = 0;	// It should authenticate from client side
		virtual int onAccept(client_t *client) = 0;			// It should authenticate from server side
};

class pluginInstanceCreator_t {
	public:
		virtual clientPlugin_t *newClient(jsonComponent_t *config) = 0;
		virtual serverPlugin_t *newServer(jsonComponent_t *config) = 0;
		virtual authenticationPlugin_t *newAuth(jsonComponent_t *config) = 0;
		virtual ~pluginInstanceCreator_t() = 0;
};

#endif
