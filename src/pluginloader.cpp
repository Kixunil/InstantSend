#include <stdio.h>
#include <stdexcept>
#include "pluginloader.h"

plugin_t pluginLoader_t::loadPlugin(string name) {
	void *handle = NULL;
	unsigned int i;
	for(i = 0; !handle && i < paths.size(); ++i) {
		handle = tryLoad(getFullName(paths[i], name));
	}
	if(!handle) throw runtime_error("Plugin not found");
	pluginInstanceCreator_t *creator = getCreator(handle);

	if(!creator) throw runtime_error("Invalid plugin");
	return plugin_t(new pluginHandle_t(handle, creator, getPluginDestructor()));
}
