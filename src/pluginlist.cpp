#include "pluginlist.h"

plugin_t &pluginList_t::operator[](const string &name) {
	if(storage.count(name)) return storage[name];
	else {
		plugin_t plugin = loader.loadPlugin(name);
		if(plugin.loaded()) return storage[name] = plugin;
		throw "can't load plugin";
	}
}

pluginList_t &pluginList_t::instance() {
	static pluginList_t *plugins;
	if(!plugins) {
		plugins = new pluginList_t();
	}

	return *plugins;
}
