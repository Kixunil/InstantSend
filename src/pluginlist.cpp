#include <stdexcept>

#include "pluginlist.h"

using namespace InstantSend;
using namespace std;

BPluginRef PluginList::operator[](const string &name) {
	/* This functon tries to insert new empty plugin handle to map.
	 * If plugin is already loaded, nothing changes and map::insert returns false as pair::second
	 * In this case, iterator to existing item is returned as pair::first
	 * If new plugin handle was inserted, plugin list tries to load library and assign it to plugin handle
	 * If load fails, plugin handle is removed from map
	 */
	modifyLock();
	// Try insert new (empty) plugin handle
	pair<map<string, pluginHandle_t>::iterator, bool> insresult(storage.insert(pair<string, pluginHandle_t>(name, pluginHandle_t())));

	// Just to make work easier
	map<string, pluginHandle_t>::iterator &plugin(insresult.first);
	if(insresult.second) { // Plugin handle was inserted
		try {
			auto_ptr<InternalPluginEnvironment> pEnv(new InternalPluginEnvironment(*instantSend, name));

			// Try load library
			pEnv->setStorageHandle(new MapPluginHandle(*this, insresult.first));
			plugin->second.assignLibrary(loader.loadPlugin(name, pEnv));
		}
		catch(...) {
			// Remove invalid plugin handle - basic exception safety
			storage.erase(plugin);

			modifyUnlock();

			// Pass exception
			throw;
		}

	}

	modifyUnlock();

	// Return base plugin reference
	return BPluginRef(plugin->second);
}

PluginList &PluginList::instance() {
	static PluginList plugins;
	return plugins;
}

void PluginList::checkUnload(const map<string, pluginHandle_t>::iterator plugin) {
	MutexHolder mh(modifyMutex);
	if(plugin->second.isUnloadable()) {
		string pname(plugin->first);
		storage.erase(plugin);
		if(mSink) mSink->sendPluginUnload(pname);
	}
}

void PluginList::MapPluginHandle::checkUnload() {
	mPluginList.checkUnload(mIter);
}

unsigned int PluginList::count() {
	MutexHolder mh(modifyMutex);
	return storage.size();
}

