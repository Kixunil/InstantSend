#include <stdexcept>

#include "pluginlist.h"
#include "appcontrol.h"

BPluginRef pluginList_t::operator[](const string &name) {
	/* This functon tries to insert new empty plugin handle to map.
	 * If plugin is already loaded, nothing changes and map::insert returns fals as pair::second
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
			// Try load library
			plugin->second.assignLibrary(loader.loadPlugin(name, plugin));
		}
		catch(exception &e) {
			// Remove invalid plugin handle - basic exception safety
			storage.erase(plugin);

			// Pass exception
			throw e;
		}

	}

	modifyUnlock();

	// Return base plugin reference
	return BPluginRef(plugin->second);
}

pluginList_t &pluginList_t::instance() {
	static pluginList_t plugins;
	return plugins;
}

/*
pluginInstanceCreator_t &getPluginInstanceCreator(const string &name) { // Interface for plugins
	pluginList_t &pl = pluginList_t::instance();
	return *pl[name].creator();
}
*/

void pluginList_t::checkUnload(const map<string, pluginHandle_t>::iterator plugin) {
	MutexHolder mh(modifyMutex);
	if(plugin->second.isUnloadable()) {
		string pname(plugin->first);
		storage.erase(plugin);
		if(mSink) mSink->sendPluginUnload(pname);
	}
}

unsigned int pluginList_t::count() {
	MutexHolder mh(modifyMutex);
	return storage.size();
}

/*
void pluginEmpty(const string &name) {
	pluginList_t::instance().checkUnload(name);
}
*/

void CheckUnloadCallback::onUnload() {
	pluginList_t::instance().checkUnload(mStorRef);
}
