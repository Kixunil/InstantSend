#include <map>
#include <string>

#include "pluginapi.h"
#ifndef PLUGINLIST
#define PLUGINLIST

#include "pluginloader.h"
#include "multithread.h"
#include "eventsink.h"

namespace InstantSend {

class PluginList { // Singleton DP
	private:
		typedef map<std::string, pluginHandle_t> Container;
		Container storage;
		PluginLoader loader;
		Mutex modifyMutex;
		EventSink *mSink;

		class MapPluginHandle : public PluginStorageHandle {
			public:
				inline MapPluginHandle(PluginList &pluginList, Container::iterator iter) : mPluginList(pluginList), mIter(iter) {}
				void checkUnload();
			private:
				PluginList &mPluginList;
				Container::iterator mIter;
		};
	public:
		inline PluginList() : mSink(NULL) {}
		BPluginRef operator[](const std::string &name);
		/*inline BPluginRef &operator[](const char *name) {
			return (*this)[std::string(name)];
		}*/
		// TODO: avoid static
		static PluginList &instance();
		inline void addSearchPath(const std::string &path) {
			loader.addPath(path);
		}

		void checkUnload(const map<std::string, pluginHandle_t>::iterator plugin);

		unsigned int count();

		inline void modifyLock() {
			modifyMutex.lock();
		}

		inline void modifyUnlock() {
			modifyMutex.unlock();
		}

		inline void setSink(EventSink &sink) {
			mSink = &sink;
		}
};

}

#endif
