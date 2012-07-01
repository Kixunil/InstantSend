#include <stdio.h>
#include "pluginloader.h"

plugin_t &pluginLoader_t::loadPlugin(string name) {
	void *handle = NULL;
	for(unsigned int i = 0; !handle && i < paths.size(); ++i) {
		handle = tryLoad(getFullName(paths[i], name));
	}
	if(!handle) throw "Plugin not found";
	pluginInstanceCreator_t *creator = getCreator(handle);
	if(!creator) throw "Invalid plugin";
	return *new plugin_t(new pluginHandle_t(handle, creator, getPluginDestructor()));
}
