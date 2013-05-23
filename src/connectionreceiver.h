#include "plugin.h"
#include "multithread.h"

extern int runningServers;
extern Mutex mRunningServers;

class connectionReceiver_t: public thread_t, serverController_t {
	public:
		connectionReceiver_t(const string &plugin, const jsonComponent_t &config);
		bool checkRunning();
		void run();
		void stop() throw();
		const string &getPluginName() throw();
		const jsonComponent_t &getPluginConf() throw();
		bool autoDelete();

	private:
		Mutex runningmutex;
		volatile bool running;
		string pluginName;
		auto_ptr<jsonComponent_t> pluginConfig;
		BPluginRef pref;
		pluginInstanceAutoPtr<serverPlugin_t> server;
};

