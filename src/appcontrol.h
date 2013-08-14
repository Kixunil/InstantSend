#include "pluginapi.h"
#include "logger.h"

#define LOG(level, message, ...) instantSend->logger().flog(level, message, ## __VA_ARGS__)

namespace InstantSend {
extern volatile bool stopApp;
extern volatile bool stopAppFast;

/*! \brief Initializes sytem specific resources 
 * \description It's called from server and client to initialize resources neccessary to perform freezeMainThread(), unfreezeMainThread() or other system specific resources like signal handlers.
 * \see onAppStop()
 * \see freezeMainThread()
 * \see unfreezeMainThread()
 * \param argc Argument count - same as main(int argc, char **argv)
 * \param argv Arguments - same as main(int argc, char **argv)
 */
void onAppStart(int argc, char **argv);

/*! \brief Frees system specific resources allocated by onAppStart
 * \see onAppStart()
 */
void onAppStop();

/*! \brief Suspends execution of main thread until unfreezeMainThread() iscalled
 * \description This function is used to prevent program from exiting. It can also receive some sort of commands like signals to control program
 * \see unfreezeMainThread()
 */
void freezeMainThread();

/*! \brief Causes main thead suspended by freezeMainThread() to continue 
 * \see freezeMainThread()
 */
void unfreezeMainThread();

class Application : public ApplicationEnvironment {
	public:
		Application(int argc, char **argv);

		const std::string &systemDataDir();
		const std::string &systemConfigDir();
		const std::string &userDataDir();
		const std::string &fileDir();

		void fileDir(const std::string &dir);

		void requestStop();
		void requestFastStop();

		inline Logger &logger() { return mLogger; }

		inline SecureStorage *secureStorage() { return mSecureStorage; }
		inline void secureStorage(SecureStorage *storage) { mSecureStorage = storage; }

	private:
		BasicLogger mLogger;
		SecureStorage *mSecureStorage;
};

extern Application *instantSend;

}
