#include "plugin.h"
#include "multithread.h"

namespace InstantSend {

extern int runningServers;
extern Mutex mRunningServers;

class ConnectionReceiver: public thread_t, ServerController {
	public:
		ConnectionReceiver(const std::string &plugin, const jsonComponent_t &config);
		bool checkRunning();
		void run();
		void stop() throw();
		const std::string &getPluginName() throw();
		const jsonComponent_t &getPluginConf() throw();
		bool autoDelete();

	private:
		Mutex runningmutex;
		volatile bool running;
		std::string pluginName;
		auto_ptr<jsonComponent_t> pluginConfig;
		BPluginRef pref;
		pluginInstanceAutoPtr<ServerPlugin> server;
};

}
