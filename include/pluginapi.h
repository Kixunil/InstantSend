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
		virtual string getMachineIdentifier() = 0;
};

class serverPlugin_t : public pluginInstance_t {
	public:
		virtual peer_t *acceptClient() = 0;
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

class connectionStatus_t {
	public:
		virtual int actualStatus();
		virtual bool isSecure() = 0;
		virtual const string &peerId() = 0;
		virtual size_t transferedBytes() = 0;
		virtual const string &pluginName() = 0;

		virtual void disconnect() = 0;
};

class pluginInstanceCreator_t {
	public:
		virtual const char *getErr() = 0;
		virtual ~pluginInstanceCreator_t();
};

class connectionCreator_t : public pluginInstanceCreator_t {
	public:
		virtual peer_t *newClient(const jsonComponent_t &config) = 0;
		virtual serverPlugin_t *newServer(const jsonComponent_t &config) = 0;
};

class asyncDataCallback_t : public peer_t {
	public:
		virtual void processData(anyData &data) = 0;

};

class asyncDataReceiver_t {
	public:
		asyncDataReceiver_t(peer_t &peer, asyncDataCallback_t &callback);
};

class securityCreator_t : public pluginInstanceCreator_t {
	public:
		virtual jsonComponent_t *getSupportedSettings(jsonComponent_t &config) = 0;
		virtual jsonComponent_t *chooseSettings(jsonComponent_t &config) = 0;
		virtual peer_t *seconnect(peer_t &peer, jsonComponent_t &config, jsonComponent_t &chosenSettings) = 0;
		virtual peer_t *seaccept(jsonComponent_t &config, jsonComponent_t &requestedConfig) = 0;
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
