#ifndef PLUGINAPI
#define PLUGINAPI

#include <stdint.h>
#include <typeinfo>
#include <malloc.h>

#include "json.h"

#define DMAXSIZE 1024

typedef struct {
	uint32_t size;
	char data[];
} anyData;

inline auto_ptr<anyData> allocData(size_t size) {
	return auto_ptr<anyData>((anyData *)operator new(sizeof(anyData) + size));
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
		//virtual string getMachineIdentifier() = 0;
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

#define IS_DIRECTION_DOWNLOAD 0
#define IS_DIRECTION_UPLOAD 1

class fileStatus_t {
	public:
		virtual string getFileName() = 0;
		virtual size_t getFileSize() = 0;
		virtual string getMachineId() = 0;
		virtual size_t getTransferredBytes() = 0;
		virtual int getTransferStatus() = 0;
		virtual int getId() = 0;
		virtual char getDirection() = 0;

		virtual void pauseTransfer() = 0;
		virtual void resumeTransfer() = 0;

		virtual ~fileStatus_t();
};

class pluginInstanceCreator_t {
	public:
		virtual const char *getErr() = 0;
		virtual ~pluginInstanceCreator_t();
};

class connectionCreator_t : public pluginInstanceCreator_t {
	public:
		virtual peer_t *newClient(jsonComponent_t *config) = 0;
		virtual serverPlugin_t *newServer(jsonComponent_t *config) = 0;
		virtual authenticationPlugin_t *newAuth(jsonComponent_t *config) = 0;
};

#define IS_TRANSFER_IN_PROGRESS 0
#define IS_TRANSFER_FINISHED 1
#define IS_TRANSFER_CANCELED_CLIENT 2
#define IS_TRANSFER_CANCELED_SERVER 3
#define IS_TRANSFER_ERROR 4

class event_t {
	public:
		virtual ~event_t();
};

class eventProgress_t : public event_t {
	public:
		virtual void onBegin(fileStatus_t &fStatus) = 0;
		virtual void onUpdate(fileStatus_t &fStatus) = 0;
		virtual void onPause(fileStatus_t &fStatus) = 0;
		virtual void onResume(fileStatus_t &fStatus) = 0;
		virtual void onEnd(fileStatus_t &fStatus) = 0;
};

class eventConnections_t : public event_t {
	public:
		virtual void onConnectionCreated(fileStatus_t &fStatus, const string &connectionIdentifier) = 0;
		virtual void onConnectionClosed(fileStatus_t &fStatus, const string &connectionIdentifier, bool unexpected) = 0;
};

class eventReceive_t : public event_t {
	public:
		virtual int onAuth() = 0;
		virtual ~eventReceive_t();
};

class eventSend_t : public event_t {
	public:
		virtual void onAuthAttemp(fileStatus_t &fStatus) = 0;
		virtual void onAuthAccept(fileStatus_t &fStatus) = 0;
		virtual void onAuthReject(fileStatus_t &fStatus) = 0;
		~eventSend_t();
};

class eventRegister_t : public event_t {
	public:
		virtual void regProgress(eventProgress_t &progressEvent) = 0;
		virtual void regReceive(eventReceive_t &receiveEvent) = 0;
		virtual void regSend(eventSend_t &sendEvent) = 0;
};

class eventHandlerCreator_t : public pluginInstanceCreator_t {
	public:
		virtual void regEvents(eventRegister_t &reg, jsonComponent_t *config) = 0;
};

pluginInstanceCreator_t &getPluginInstanceCreator(const string &name); // Plugins can use other plugins

#endif
