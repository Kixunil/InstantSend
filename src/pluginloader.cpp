#include <stdio.h>
#include <stdexcept>
#include "pluginloader.h"

//void pluginEmpty(const string &name);

//void CheckUnloadCallback::onUnload() {
//	pluginEmpty(name);
//}

LibraryHandleWithCallback::~LibraryHandleWithCallback() {}

void LibraryHandleWithCallback::onUnload() {
	mCallback->onUnload();
}

auto_ptr<LibraryHandle> pluginLoader_t::loadPlugin(const string &name, const CheckUnloadCallback::StorageRef &storageRef) {
	unsigned int i;
	for(i = 0; i < paths.size(); ++i) {
		try {
			fprintf(stderr, "Loading: %s\n", getFullName(paths[i], name).c_str());
			return tryLoad(getFullName(paths[i], name), storageRef);
		}
		catch(exception &e) {
			fprintf(stderr, "Failed to load plugin %s from path %s (%s)", name.c_str(), getFullName(paths[i], name).c_str(), e.what());
		}
	}
	throw runtime_error("Failed to load plugin %s");
}
