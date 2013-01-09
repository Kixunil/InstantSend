#include "pluginapi.h"
#include "multithread.h"

class connectionReceiver_t: public thread_t, serverController_t {
	public:
		connectionReceiver_t(const string &plugin, const jsonComponent_t &config);
		bool checkRunning();
		void run();
		void stop();
		const string &getPluginName();
		const jsonComponent_t &getPluginConf();
		bool autoDelete();

	private:
		auto_ptr<mutex_t> runningmutex;
		volatile bool running;
		string pluginName;
		auto_ptr<jsonComponent_t> pluginConfig;
		auto_ptr<serverPlugin_t> server;
};

