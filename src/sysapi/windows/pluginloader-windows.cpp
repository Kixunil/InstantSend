#include <windows.h>
#include <stdexcept>
#include <stdio.h>

#include "pluginloader.h"

string pluginLoader_t::getFullName(const string &path, const string &name) {
	return path + "\\" + name + ".dll";
}

void *pluginLoader_t::tryLoad(const string &path) {
	fprintf(stderr, "Loading: %s\n", path.c_str());
	HMODULE *lib = new HMODULE;
	*lib = LoadLibrary(path.c_str());
	if(*lib) return (void *)lib; else {
		delete lib;
		return NULL;
	}
}

typedef pluginInstanceCreator_t *(*getCreatorFunc)(void);

pluginInstanceCreator_t *pluginLoader_t::getCreator(void *handle) {
	FARPROC gc = GetProcAddress(*(HMODULE *)handle, "getCreator");
	if(!gc) throw runtime_error("Invalid plugin");
	getCreatorFunc creator = (getCreatorFunc)gc;
	return creator();
}

void closePlugin(void *handle) {
	FreeLibrary(*(HMODULE *)handle);
	delete (HMODULE *)handle;
}

void (*pluginLoader_t::getPluginDestructor())(void *) {
	return &closePlugin;
}

