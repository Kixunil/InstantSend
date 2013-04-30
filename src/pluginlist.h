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
		auto_ptr<mutex_t> modifyMutex;
		eventSink_t *mSink;
	public:
		inline pluginList_t() : modifyMutex(mutex_t::getNew()), mSink(NULL) {}
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
			modifyMutex->get();
		}

		inline void modifyUnlock() {
			modifyMutex->release();
		}

		inline void setSink(eventSink_t &sink) {
			mSink = &sink;
		}

		/*inline void accessLock() {
			modifyMutex->get();
		}

		inline void accessUnlock() {
			modifyMutex->release();
		}*/
};

#endif
