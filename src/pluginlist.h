#include <map>
#include <string>

#include "pluginapi.h"
#ifndef PLUGINLIST
#define PLUGINLIST

#include "pluginloader.h"
#include "multithread.h"
#include "eventsink.h"

using namespace std;

class pluginList_t { // Singleton DP
	private:
		map<string, pluginHandle_t> storage;
		pluginLoader_t loader;
		Mutex modifyMutex;
		eventSink_t *mSink;
	public:
		inline pluginList_t() : mSink(NULL) {}
		BPluginRef operator[](const string &name);
		/*inline BPluginRef &operator[](const char *name) {
			return (*this)[string(name)];
		}*/
		static pluginList_t &instance();
		inline void addSearchPath(const string &path) {
			loader.addPath(path);
		}

		void checkUnload(const map<string, pluginHandle_t>::iterator plugin);

		unsigned int count();

		inline void modifyLock() {
			modifyMutex.lock();
		}

		inline void modifyUnlock() {
			modifyMutex.unlock();
		}

		inline void setSink(eventSink_t &sink) {
			mSink = &sink;
		}
};

#endif
