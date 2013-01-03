#include <dlfcn.h>
#include <stdexcept>
#include <stdio.h>

#include "pluginloader.h"

string pluginLoader_t::getFullName(const string &path, const string &name) {
	return path + "/" + name + ".so";
}

void *pluginLoader_t::tryLoad(const string &path) {
	fprintf(stderr, "Loading: %s\n", path.c_str());
	return dlopen(path.c_str(), RTLD_LAZY);
}


pluginInstanceCreator_t *pluginLoader_t::getCreator(void *handle) {
	pluginInstanceCreator_t *(* creator)();
	*(void **)(&creator) = dlsym(handle, "getCreator");
	if(!creator) throw runtime_error("Invalid plugin");
	return creator();
}

void closePlugin(void *handle) {
	dlclose(handle);
}

void (*pluginLoader_t::getPluginDestructor())(void *) {
	return &closePlugin;
}

