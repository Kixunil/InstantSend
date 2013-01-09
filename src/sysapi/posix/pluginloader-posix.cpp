#include <dlfcn.h>
#include <stdexcept>
#include <stdio.h>

#include "pluginloader.h"

string pluginLoader_t::getFullName(const string &path, const string &name) {
	return path + "/" + name + LIBRARY_EXTENSION;
}

void *pluginLoader_t::tryLoad(const string &path) {
	fprintf(stderr, "Loading: %s\n", path.c_str());
	void *result = dlopen(path.c_str(), RTLD_LAZY);
	if(!result) throw runtime_error(dlerror());
	return result;
}


pluginInstanceCreator_t *pluginLoader_t::getCreator(void *handle, const string &name) {
	pluginInstanceCreator_t *(* creator)(pluginEmptyCallback_t &);
	*(void **)(&creator) = dlsym(handle, "getCreator");
	if(!creator) throw runtime_error("Invalid plugin");
	auto_ptr<checkUnloadCallback_t> callback(new checkUnloadCallback_t(name));
	pluginInstanceCreator_t *result = creator(*callback);
	callback.release();
	return result;
}

void closePlugin(void *handle) {
	dlclose(handle);
}

void (*pluginLoader_t::getPluginDestructor())(void *) {
	return &closePlugin;
}

