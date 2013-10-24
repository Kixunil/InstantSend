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
	#define DMAXSIZE 65545 // It's 65536 + 8 + 1, so files can be read in multiples of 4096 (usual sector size); 8 is sizeof(File::Size) 1 is for leading 0
#endif

#ifndef OLD_DMAXSIZE
	#define OLD_DMAXSIZE 1024 // Old maximum size for backward compatibility
#endif

namespace InstantSend {

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
inline std::auto_ptr<anyData> allocData(size_t size) {
	return std::auto_ptr<anyData>((anyData *)operator new(sizeof(anyData) + size));
}

class pluginDestrCallback {
	public:
};

class Logger {
	public:
		enum Level {
			FatalError = 0,
			SecurityError = 1,
			Error = 2,
			SecurityWarning = 3,
			Warning = 4,
			Note = 5,
			Debug = 6,
			VerboseDebug = 7
		};

		virtual void log(Level level, const std::string &message) = 0;
		virtual void log(Level level, const std::string &message, const std::string &pluginName) = 0;
		void flog(Level level, const char * format, ...);

		virtual ~Logger();

		static const char *LevelToStr(Level level);
};

/*! Interface for management of secrets (encryption keys, passwords...)
 */
class SecureStorage {
	public:
		/*! \return True if storage is locked (encrypted). */
		virtual bool isLocked() = 0;

		/*! Requests unlocking. This can ask user for password.
		 * \return True if unlocking succeeded. */
		virtual bool reuqestUnlock() = 0;

		/*! Attemps to retrieve secret from storage.
		 * \param name Meaningful name of secret. (example: "master password")
		 * \param value Secret will be stored here
		 * \param silent If true and storage is locked, retrieving should fail without asking user for password.
		 * \return True if secret was successfuly retrieved
		 * \note Plugins don't have to worry about name conflicts with other plugins or application. They will be automatically resolved by application.
		 */
		virtual bool getSecret(const std::string &name, std::string &value, bool silent = false) = 0;

		/*! Attemps to store secret.
		 * \param name Meaningful name of secret. (example: "master password")
		 * \param value Secret to be stored
		 * \param silent If true and storage is locked, retrieving should fail without asking user for password.
		 * \return True if secret was successfuly stored
		 */
		virtual bool setSecret(const std::string &name, const std::string &value, bool silent = false) = 0;

		/*! Attemps to remove secret from storage
		 * \param name Meaningful name of secret. (example: "master password")
		 * \return True if secret was successfuly removed
		 */
		virtual bool removeSecret(const std::string &name) = 0;
};

/* Provides global information and functions from application, operating system or other plugins. */
class ApplicationEnvironment {
	public:
		virtual const std::string &systemDataDir() = 0;
		virtual const std::string &systemConfigDir() = 0;
		virtual const std::string &userDataDir() = 0;
		virtual const std::string &fileDir() = 0;

		virtual void fileDir(const std::string &dir) = 0;

		virtual void requestStop() = 0;
		virtual void requestFastStop() = 0;
};

/* Provides plugin specific information and functions from application, operating system or other plugins. */
class PluginEnvironment {
	public:
		ApplicationEnvironment &mAppEnv;

		virtual const std::string &systemPluginDataDir() = 0;
		virtual const std::string &systemPluginConfigDir() = 0;
		virtual const std::string &userPluginDir() = 0;

		virtual void onInstanceCreated() = 0;
		virtual void onInstanceDestroyed() = 0;

		virtual void log(Logger::Level level, const std::string &message) = 0;
		void flog(Logger::Level level, const char *format, ...);

		/* Secure storage if available. (NULL if not)*/
		virtual SecureStorage *secureStorage() = 0;

	protected:
		inline PluginEnvironment(ApplicationEnvironment &appEnv) : mAppEnv(appEnv) {}
};

/*! \brief Base for creator of plugin instances */
class PluginInstanceCreator {
	public:
		inline PluginInstanceCreator(PluginEnvironment &pluginEnv) : mEnv(pluginEnv) {}
		virtual ~PluginInstanceCreator();
		PluginEnvironment &mEnv;
};

/*! \brief Plugin can provide user-friendly initialization (or deletion) of configuration using this (optional) interface.
 */
class PluginConfigInitializer : public PluginInstanceCreator {
	virtual void initUserConfig(jsonObj_t &globalConfig, const jsonComponent_t *systemConfig, bool server) = 0;
	virtual void clearUserConfig(jsonObj_t &globalConfig, bool server) = 0;
};

/*! \brief Ancestor of all plugin instances.
 * All plugin instances should inherit from this class.
 */
class PluginInstance {
	public:
		/*! \brief Default constructor */
		inline PluginInstance(PluginEnvironment &env) : mEnv(env) { env.onInstanceCreated(); }

		/*! \brief Destructor made virtual.
		 *
		 * Purpose is, instance can be easily deleted anywhere in the code.
		 */
		virtual ~PluginInstance();

		PluginEnvironment &mEnv;
};

/*! \brief Interface used for communication with remote machines
 */
class Peer : public PluginInstance {
	public:
		inline Peer(PluginEnvironment &env) : PluginInstance(env) {}

		/*! \brief Sends chunk of data to remote machine.
		 *
		 * This function should send chunk of data using arbitrary communication channel.
		 * \attention The data MUST come out EXACTLY same as were sent!
		 * \note This function should block until chunk is sent.
		 * \see Peer::recvData
		 * \param data Chunk of data to be sent.
		 * \return 1 if sending succeeded, 0 if failed
		 */
		virtual int sendData(anyData *data) = 0;

		/*! \brief Receives chunk of data from remote machine.
		 *
		 * This function should receive chunk of data using arbitrary communication channel.
		 * \attention The data MUST come out EXACTLY same as were sent!
		 * This function MUST block until chunk is received or error detected!
		 * \see Peer::sendData
		 * \param data Pre-allocated chunk of data. anyData::size is initialised to maximum size of data, chunk can hold.
		 * \return 1 if receivinging succeeded, 0 if failed or connection is closed (no more data)
		 */
		virtual int recvData(anyData *data) = 0;

		/*! \return Human-readable std::string which identifies remote machine. (like "192.168.1.42:42000")
		 */
		virtual std::string getMachineIdentifier() = 0;
};


class SecureStorageCreator : public PluginInstanceCreator {
	public:
		inline SecureStorageCreator(PluginEnvironment &pluginEnv) : PluginInstanceCreator(pluginEnv) {}
		virtual SecureStorage *openSecureStorage(const jsonComponent_t *config) = 0;
		virtual void closeSecureStorage(SecureStorage &secureStorage) = 0;
};

/*! \brief Server interface
 */
class ServerPlugin : public PluginInstance {
	public:
		inline ServerPlugin(PluginEnvironment &env) : PluginInstance(env) {}
		/*! \brief Waits for client to connect.
		 * \return Instance of Peer which should communicate with connected client
		 * \attention This function MUST block!
		 */
		virtual std::auto_ptr<Peer> acceptClient() throw() = 0;
};

/*! \brief Interface for asynchronously stopping server or Peer
 * \description This is optionl feature of plugin but good quality plugin should have it. It's used when user decides to unload plugin, cancel transfer or stop application. This prevents delays.
 */
class AsyncStop {
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
class FileStatus {
	public:
		/*! \return Full path to file
		 */
		virtual std::string getFileName() = 0;

		/*! \return File size in bytes
		 */
		virtual File::Size getFileSize() = 0;

		/*! \return Human readable std::string, which identifies remote machine
		 */
		virtual std::string getMachineId() = 0;

		/*! \return Number of received/sent bytes of file (NOT inluding communication headers)
		 */
		virtual File::Size getTransferredBytes() = 0;

		/*! \return IS_TRANSFER_IN_PROGRESS, if file hasn't been transferred yet;
		 * \return IS_TRANSFER_FINISHED, if file has been transferred (FileStatus will be deleted soon);
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

		virtual ~FileStatus();
};

class connectionStatus {
	public:
		virtual int actualStatus();

		/*! \return true if connection is encrypted */
		virtual bool isSecure() = 0;

		/*! \return Human readable identifier of remote machine */
		virtual const std::string &peerId() = 0;

		/*! \return Number of bytes transferred using this connection */
		virtual File::Size transferedBytes() = 0;

		/*! \return Human readable name of plugin */
		virtual const std::string &pluginName() = 0;

		/*! \brief forces connection closing */
		virtual void disconnect() = 0;
};

/*! \brief Creator of server and client instances */
class ConnectionCreator : public PluginInstanceCreator {
	public:
		inline ConnectionCreator(PluginEnvironment &env) : PluginInstanceCreator(env) {}
		/*! \brief Connects to server 
		 *  \return Instance of Peer, which will communicate with server
		 */
		virtual std::auto_ptr<Peer> newClient(const jsonComponent_t &config) throw() = 0;

		/*! \brief Initialises server
		 * \return Instance of serverPlugin, which is ready to accept clients
		 */
		virtual std::auto_ptr<ServerPlugin> newServer(const jsonComponent_t &config) throw() = 0;
};

/*! \brief Creates encryption instances
 */
class SecurityCreator : public PluginInstanceCreator {
	public:
		inline SecurityCreator(PluginEnvironment &env) throw() : PluginInstanceCreator(env) {}

		/*! \brief Initiates secure connection from client side
		 * \param peer Instance of Peer connected to server. 
		 * \param config User defined configuration
		 * \return instance of Peer, which encrypts communication between server and client
		 */
		virtual std::auto_ptr<Peer> seconnect(Peer &peer, jsonComponent_t &config) throw() = 0;

		/*! \brief Waits for client to initiate secure connection
		 * \param config User defined configuration
		 * \return instance of Peer, which encrypts communication between server and client
		 */
		virtual std::auto_ptr<Peer> seaccept(Peer &peer, jsonComponent_t &config) throw() = 0;
};

#define IS_TRANSFER_IN_PROGRESS 0
#define IS_TRANSFER_FINISHED 1
#define IS_TRANSFER_CANCELED_CLIENT 2
#define IS_TRANSFER_CANCELED_SERVER 3
#define IS_TRANSFER_ERROR 4
#define IS_TRANSFER_CONNECTING 5

/*! \brief Interface for progress event handlers
 */
class EventProgress {
	public:
		/*! \brief This event is called every time, when transfer started
		 * \param fStatus Reference to file controller assigned to file
		 */
		virtual void onBegin(FileStatus &fStatus) throw() = 0;

		/*! \brief This event is called periodically, during transfer, if status of file was updated.
		 * \note It's not guaranteed, that this will be called after each update (write to file). It may be called less often, so plugins won't be spammed. In case of small files, it may bo not called.
		 * \param fStatus Reference to file controller assigned to file
		 */
		virtual void onUpdate(FileStatus &fStatus) throw() = 0;

		/*! \brief This event is called, when transfer was paused 
		 * \param fStatus Reference to file controller assigned to file
		 */
		virtual void onPause(FileStatus &fStatus) throw() = 0;

		/*! \brief This event is called, when (previously paused) transfer was resumed
		 * \param fStatus Reference to file controller assigned to file
		 */
		virtual void onResume(FileStatus &fStatus) throw() = 0;

		/*! \brief This event is called every time, when transfer ended
		 * \param fStatus Reference to file controller assigned to file
		 */
		virtual void onEnd(FileStatus &fStatus) throw() = 0;
};

class EventConnections {
	public:
		/*! \brief This event is called every time, when connetcion is created
		 * \param fStatus File which is associated with connection
		 * \param connectionIdentifier Human readable description of connection
		 */
		virtual void onConnectionCreated(FileStatus &fStatus, const std::string &connectionIdentifier) throw() = 0;

		/*! \brief This event is called every time, when connetcion is closed
		 * \param fStatus File which is associated with connection
		 * \param connectionIdentifier Human readable description of connection
		 * \param unexpected True, if connection is lost during file transfer, flase if file was successfully transfered
		 */
		virtual void onConnectionClosed(FileStatus &fStatus, const std::string &connectionIdentifier, bool unexpected) throw() = 0;
};

class EventSend {
	public:
		virtual void onAuthAttemp(FileStatus &fStatus) throw() = 0;
		virtual void onAuthAccept(FileStatus &fStatus) throw() = 0;
		virtual void onAuthReject(FileStatus &fStatus) throw() = 0;
};

class ServerController {
	public:
		virtual const std::string &getPluginName() throw() = 0;
		virtual const jsonComponent_t &getPluginConf() throw() = 0;

		//virtual bool stop() throw() = 0; // use : public asyncStop()
};

class EventServer {
	public:
		virtual void onStarted(ServerController &server) throw() = 0;
		virtual void onStopped(ServerController &server) throw() = 0;
};

class EventPlugin {
	public:
		virtual void onLoad(const std::string &pluginName) = 0;
		virtual void onUnload(const std::string &pluginName) = 0;
};

/*! \brief Interface for registering events
 * \details Plugins can use this interface to register their event handlers
 */
class EventRegister {
	public:
		/*! \brief Registers progress event
		 * \param progressEvent Event handler for progress event
		 */
		virtual void regProgress(EventProgress &progressEvent) throw() = 0;

		/*! \brief Unregisters progress event
		 * \param progressEvent Event handler for progress event
		 */
		virtual void unregProgress(EventProgress &progressEvent) throw() = 0;

#if 0
		/*! \brief Registers receive event
		 * \param receiveEvent Event handler for receive event
		 */
		virtual void regReceive(EventReceive &receiveEvent) throw() = 0;

		/*! \brief Unregisters receive event
		 * \param receiveEvent Event handler for receive event
		 */
		virtual void unregReceive(EventReceive &receiveEvent) throw() = 0;

		/*! \brief Registers send event
		 * \param sendEvent Event handler for send event
		 */
		virtual void regSend(EventSend &sendEvent) throw() = 0;

		/*! \brief Unregisters send event
		 * \param sendEvent Event handler for send event
		 */
		virtual void unregSend(EventSend &sendEvent) throw() = 0;
#endif

		/*! \brief Registers server event
		 * \param serverEvent Event handler for server event
		 */
		virtual void regServer(EventServer &serverEvent) throw() = 0;

		/*! \brief Unregisters server event
		 * \param serverEvent Event handler for server event
		 */
		virtual void unregServer(EventServer &serverEvent) throw() = 0;
};

/*! brief Interface for registering events
 */
class EventHandlerCreator : public PluginInstanceCreator {
	public:
		inline EventHandlerCreator(PluginEnvironment &env) : PluginInstanceCreator(env) {}
		/*! \brief Tells plugin to register its' events
		 * \details This method is called after plugin is loaded, so it can create event handlers and register them.
		 * \param reg Interface for registering events
		 * \param config User configuration stored in Json
		 */
		virtual void regEvents(EventRegister &reg, jsonComponent_t *config) throw() = 0;

		/*! \brief Tells plugin to unregister all its' registered events
		 * \details This method is called before plugin is unloaded.
		 */
		virtual void unregEvents(EventRegister &reg) throw() = 0;
};

} // namespace InstantSend

#endif
