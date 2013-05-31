#ifndef PLUGINAPI
#define PLUGINAPI

#include <stdint.h>
#include <typeinfo>
#include <memory>
#include <stdexcept>
#include <string>

#include "file.h"
#include "json.h"

#ifndef DMAXSIZE
	#define DMAXSIZE 1024
#endif

/*! \brief Struct defining chunk of binary data
 */

typedef struct {
	/*! \brief Size of data in chunk */
	uint32_t size;
	/*! \brief Actual binary data */
	char data[];
} anyData;

/*! \brief Allocates chunk of data.
 * \param size Maximum size of data, chunk can hold.
 * \return Auto pointer to allocated data chunk.
*/
inline auto_ptr<anyData> allocData(size_t size) {
	return auto_ptr<anyData>((anyData *)operator new(sizeof(anyData) + size));
}

class pluginDestrCallback_t {
	public:
		virtual void onUnload() = 0;
};

/*! \brief Base for creator of plugin instances */
class pluginInstanceCreator_t {
	public:
		/*! \return Human readable description of last error */
		virtual const char *getErr() = 0;
		virtual ~pluginInstanceCreator_t();
		virtual bool unloadPossible();
};

class pluginMultiInstanceCreator_t : public pluginInstanceCreator_t {
	public:
		inline pluginMultiInstanceCreator_t(pluginDestrCallback_t &destrCallback) : count(0), callback(destrCallback) {}
		inline void inc() { ++count; }
		inline void dec() { --count; }
		bool unloadPossible();
		inline void checkUnload() { if(!count) callback.onUnload(); }
		~pluginMultiInstanceCreator_t();
	private:
		unsigned int count;
		pluginDestrCallback_t &callback;
};

/*! \brief Ancestor of all plugin instances.
 * All plugin instances should inherit from this class.
 */
class pluginInstance_t {
	public:
		/*! \brief Default constructor */
		inline pluginInstance_t(pluginMultiInstanceCreator_t &instanceCreator) : instcreator(instanceCreator) { instcreator.inc(); }

#if 0
		/*! \brief change configuration during run-time.
		 *
		 * This function has no purpose now and I'm unsure, if it'll have. It may be removed in the future.
		 * \param config New configuration of plugin instance.
		 */
		virtual void reconfigure(jsonComponent_t *config) = 0;
#endif

		/*! \brief Destructor made virtual.
		 *
		 * Purpose is, instance can be easily deleted anywhere in the code.
		 */
		virtual ~pluginInstance_t();
		inline pluginMultiInstanceCreator_t &getCreator() { return instcreator; }
	private:
		pluginMultiInstanceCreator_t &instcreator;
};

//template <class >

/*! \brief Interface used for communication with remote machines
 */
class peer_t : public pluginInstance_t {
	public:
		inline peer_t(pluginMultiInstanceCreator_t &creator) : pluginInstance_t(creator) {}

		/*! \brief Sends chunk of data to remote machine.
		 *
		 * This function should send chunk of data using arbitrary communication channel.
		 * \attention The data MUST come out EXACTLY same as were sent!
		 * \note This function should block until chunk is sent.
		 * \see peer_t::recvData
		 * \param data Chunk of data to be sent.
		 * \return 1 if sending succeeded, 0 if failed
		 */
		virtual int sendData(anyData *data) = 0;

		/*! \brief Receives chunk of data from remote machine.
		 *
		 * This function should receive chunk of data using arbitrary communication channel.
		 * \attention The data MUST come out EXACTLY same as were sent!
		 * This function MUST block until chunk is received or error detected!
		 * \see peer_t::sendData
		 * \param data Pre-allocated chunk of data. anyData::size is initialised to maximum size of data, chunk can hold.
		 * \return 1 if receivinging succeeded, 0 if failed or connection is closed (no more data)
		 */
		virtual int recvData(anyData *data) = 0;

		/*! \return Human-readable string which identifies remote machine. (like "192.168.1.42:42000")
		 */
		virtual string getMachineIdentifier() = 0;
};

/*! \brief Server interface
 */
class serverPlugin_t : public pluginInstance_t {
	public:
		inline serverPlugin_t(pluginMultiInstanceCreator_t &creator) : pluginInstance_t(creator) {}
		/*! \brief Waits for client to connect.
		 * \return Instance of peer_t which should communicate with connected client
		 * \attention This function MUST block!
		 */
		virtual auto_ptr<peer_t> acceptClient() throw() = 0;
};

/*! \brief Interface for asynchronously stopping server or peer
 * \description This is optionl feature of plugin but good quality plugin should have it. It's used when user decides to unload plugin, cancel transfer or stop application. This prevents delays.
 */
class asyncStop_t {
	public:
		/*! \brief Stops waiting for connection or data
		 * \description If plugin is executing function, which waits for something (connection or data), calling of this function (from other thread) should cause it to return. If plugin isn't executing such function, calling of stop() should cause next call to such function return immediately, to prevent race conditions.
		 * If such function was interrupted by stop(), it should return NULL (meaning no data/connection received)
		 * \return false if stop failed (for sure), true if stop succeeded or it's unknown.
		 */
		virtual bool stop() = 0;
};

#define IS_DIRECTION_DOWNLOAD 0
#define IS_DIRECTION_UPLOAD 1

/*! \brief interface for controlling file transfer
 */
class fileStatus_t {
	public:
		/*! \return Full path to file
		 */
		virtual string getFileName() = 0;

		/*! \return File size in bytes
		 */
		virtual File::Size getFileSize() = 0;

		/*! \return Human readable string, which identifies remote machine
		 */
		virtual string getMachineId() = 0;

		/*! \return Number of received/sent bytes of file (NOT inluding communication headers)
		 */
		virtual File::Size getTransferredBytes() = 0;

		/*! \return IS_TRANSFER_IN_PROGRESS, if file hasn't been transferred yet;
		 * \return IS_TRANSFER_FINISHED, if file has been transferred (fileStatus will be deleted soon);
		 * \return IS_TRANSFER_CANCELED_CLIENT, if user on client side has cancelled file transfer;
		 * \return IS_TRANSFER_CANCELED_SERVER, if user on server side has cancelled file transfer;
		 * \return IS_TRANSFER_ERROR, if file transfer was cancelled due to error (mainly lost connection)
		 */
		virtual int getTransferStatus() = 0;

		/*! \return Unique number which identifies file
		 */
		virtual int getId() = 0;

		/*! \return IS_DIRECTION_DOWNLOAD, if receiving file;
		 * \return IS_DIRECTION_UPLOAD, if sending file;
		 */
		virtual char getDirection() = 0;

		/*! \brief Pauses file transfer until resumeTransfer() is called */
		virtual void pauseTransfer() = 0;

		/*! \brief Resumes file transfer if it was paused by pauseTransfer() call */
		virtual void resumeTransfer() = 0;

		virtual ~fileStatus_t();
};

class connectionStatus_t {
	public:
		virtual int actualStatus();

		/*! \return true if connection is encrypted */
		virtual bool isSecure() = 0;

		/*! \return Human readable identifier of remote machine */
		virtual const string &peerId() = 0;

		/*! \return Number of bytes transferred using this connection */
		virtual size_t transferedBytes() = 0;

		/*! \return Human readable name of plugin */
		virtual const string &pluginName() = 0;

		/*! \brief forces connection closing */
		virtual void disconnect() = 0;
};

/*! \brief Creator of server and client instances */
class connectionCreator_t : public pluginMultiInstanceCreator_t {
	public:
		inline connectionCreator_t(pluginDestrCallback_t &callback) : pluginMultiInstanceCreator_t(callback) {}
		/*! \brief Connects to server 
		 *  \return Instance of peer_t, which will communicate with server
		 */
		virtual auto_ptr<peer_t> newClient(const jsonComponent_t &config) throw() = 0;

		/*! \brief Initialises server
		 * \return Instance of serverPlugin_t, which is ready to accept clients
		 */
		virtual auto_ptr<serverPlugin_t> newServer(const jsonComponent_t &config) throw() = 0;
};

/*! \brief Interface for processing data
 * \details If class implements this interface, it can be passed to asyncDataReceiver_t
 */
class asyncDataCallback_t : public peer_t {
	public:
		virtual void processData(anyData &data) = 0;

};

class asyncDataReceiver_t {
	public:
		asyncDataReceiver_t(peer_t &peer, asyncDataCallback_t &callback);
};

/*! \brief Creates encryption instances
 */
class securityCreator_t : public pluginMultiInstanceCreator_t {
	public:
		inline securityCreator_t(pluginDestrCallback_t &callback) throw() : pluginMultiInstanceCreator_t(callback) {}

		/*! \brief Publishes supported encryption settings
		 * \details Encryption plugins can support multiple settings. Return value of this function is delivered to client, during handshake. Client can choose settings (method chooseSettings(() is called) and reply is sent back to server.
		 * \see chooseSettings()
		 * \param config User defined configuration
		 * \return JSON component containing informations for client
		 */
		virtual auto_ptr<jsonComponent_t> getSupportedSettings(jsonComponent_t &config) throw() = 0;

		/*! \brief Publishes supported encryption settings
		 * \details Encryption plugins can support multiple settings. Return value of getSupportedSettings() is delivered to client, where this method is called, to choose settings. Reply is sent to server.
		 * \see getSupportedSettings()
		 * \param config Supported configurations sent by server
		 * \return JSON component containing choosen configuration
		 */
		virtual auto_ptr<jsonComponent_t> chooseSettings(jsonComponent_t &config) throw() = 0;

		/*! \brief Initiates secure connection from client side
		 * \param peer Instance of peer_t connected to server. 
		 * \param config User defined configuration
		 * \param chosenSettings Settings required by chooseSettings() call
		 * \return instance of peer_t, which encrypts communication between server and client
		 */
		virtual auto_ptr<peer_t> seconnect(peer_t &peer, jsonComponent_t &config, jsonComponent_t &chosenSettings) throw() = 0;

		/*! \brief Waits for client to initiate secure connection
		 * \param config User defined configuration
		 * \param requestedConfig Settings requested by client
		 * \return instance of peer_t, which encrypts communication between server and client
		 */
		virtual auto_ptr<peer_t> seaccept(peer_t &peer, jsonComponent_t &config, jsonComponent_t &requestedConfig) throw() = 0;
};

#define IS_TRANSFER_IN_PROGRESS 0
#define IS_TRANSFER_FINISHED 1
#define IS_TRANSFER_CANCELED_CLIENT 2
#define IS_TRANSFER_CANCELED_SERVER 3
#define IS_TRANSFER_ERROR 4

/*! \brief Interface for progress event handlers
 */
class eventProgress_t {
	public:
		/*! \brief This event is called every time, when transfer started
		 * \param fStatus Reference to file controller assigned to file
		 */
		virtual void onBegin(fileStatus_t &fStatus) throw() = 0;

		/*! \brief This event is called periodically, during transfer, if status of file was updated.
		 * \note It's not guaranteed, that this will be called after each update (write to file). It may be called less often, so plugins won't be spammed. In case of small files, it may bo not called.
		 * \param fStatus Reference to file controller assigned to file
		 */
		virtual void onUpdate(fileStatus_t &fStatus) throw() = 0;

		/*! \brief This event is called, when transfer was paused 
		 * \param fStatus Reference to file controller assigned to file
		 */
		virtual void onPause(fileStatus_t &fStatus) throw() = 0;

		/*! \brief This event is called, when (previously paused) transfer was resumed
		 * \param fStatus Reference to file controller assigned to file
		 */
		virtual void onResume(fileStatus_t &fStatus) throw() = 0;

		/*! \brief This event is called every time, when transfer ended
		 * \param fStatus Reference to file controller assigned to file
		 */
		virtual void onEnd(fileStatus_t &fStatus) throw() = 0;
};

class eventConnections_t {
	public:
		/*! \brief This event is called every time, when connetcion is created
		 * \param fStatus File which is associated with connection
		 * \param connectionIdentifier Human readable description of connection
		 */
		virtual void onConnectionCreated(fileStatus_t &fStatus, const string &connectionIdentifier) throw() = 0;

		/*! \brief This event is called every time, when connetcion is closed
		 * \param fStatus File which is associated with connection
		 * \param connectionIdentifier Human readable description of connection
		 * \param unexpected True, if connection is lost during file transfer, flase if file was successfully transfered
		 */
		virtual void onConnectionClosed(fileStatus_t &fStatus, const string &connectionIdentifier, bool unexpected) throw() = 0;
};

class eventReceive_t {
	public:
		virtual int onAuth() throw() = 0;
		virtual ~eventReceive_t();
};

class eventSend_t {
	public:
		virtual void onAuthAttemp(fileStatus_t &fStatus) throw() = 0;
		virtual void onAuthAccept(fileStatus_t &fStatus) throw() = 0;
		virtual void onAuthReject(fileStatus_t &fStatus) throw() = 0;
		~eventSend_t();
};

class serverController_t {
	public:
		virtual const string &getPluginName() throw() = 0;
		virtual const jsonComponent_t &getPluginConf() throw() = 0;

		//virtual bool stop() throw() = 0; // use : public asyncStop()
};

class eventServer_t {
	public:
		virtual void onStarted(serverController_t &server) throw() = 0;
		virtual void onStopped(serverController_t &server) throw() = 0;
};

class eventPlugin_t {
	public:
		virtual void onLoad(const string &pluginName) = 0;
		virtual void onUnload(const string &pluginName) = 0;
};

/*! \brief Interface for registering events
 * \details Plugins can use this interface to register their event handlers
 */
class eventRegister_t {
	public:
		/*! \brief Registers progress event
		 * \param progressEvent Event handler for progress event
		 */
		virtual void regProgress(eventProgress_t &progressEvent) throw() = 0;

		/*! \brief Unregisters progress event
		 * \param progressEvent Event handler for progress event
		 */
		virtual void unregProgress(eventProgress_t &progressEvent) throw() = 0;

		/*! \brief Registers receive event
		 * \param receiveEvent Event handler for receive event
		 */
		virtual void regReceive(eventReceive_t &receiveEvent) throw() = 0;

		/*! \brief Unregisters receive event
		 * \param receiveEvent Event handler for receive event
		 */
		virtual void unregReceive(eventReceive_t &receiveEvent) throw() = 0;

		/*! \brief Registers send event
		 * \param sendEvent Event handler for send event
		 */
		virtual void regSend(eventSend_t &sendEvent) throw() = 0;

		/*! \brief Unregisters send event
		 * \param sendEvent Event handler for send event
		 */
		virtual void unregSend(eventSend_t &sendEvent) throw() = 0;

		/*! \brief Registers server event
		 * \param serverEvent Event handler for server event
		 */
		virtual void regServer(eventServer_t &serverEvent) throw() = 0;

		/*! \brief Unregisters server event
		 * \param serverEvent Event handler for server event
		 */
		virtual void unregServer(eventServer_t &serverEvent) throw() = 0;
};

/*! brief Interface for registering events
 */
class eventHandlerCreator_t : public pluginInstanceCreator_t {
	public:
		/*! \brief Tells plugin to register its' events
		 * \details This method is called after plugin is loaded, so it can create event handlers and register them.
		 * \param reg Interface for registering events
		 * \param config User configuration stored in Json
		 */
		virtual void regEvents(eventRegister_t &reg, jsonComponent_t *config) throw() = 0;

		/*! \brief Tells plugin to unregister all its' registered events
		 * \details This method is called before plugin is unloaded.
		 */
		virtual void unregEvents(eventRegister_t &reg) throw() = 0;
};

/*! \brief Loads other plugin
 * \param name File name of plugin without extension.
 * \return Instance of pluginInstanceCreator_t
 */
//pluginInstanceCreator_t &getPluginInstanceCreator(const string &name); // Plugins can use other plugins

#endif
