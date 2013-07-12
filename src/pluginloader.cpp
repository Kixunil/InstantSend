#include <stdio.h>
#include <stdexcept>
#include "pluginloader.h"

using namespace InstantSend;

//void pluginEmpty(const string &name);

//void CheckUnloadCallback::onUnload() {
//	pluginEmpty(name);
//}
//

// TODO implement
void LibraryHandleWithEnvironment::onUnload() {}

LibraryHandleWithEnvironment::~LibraryHandleWithEnvironment() {}


auto_ptr<LibraryHandle> PluginLoader::loadPlugin(const string &name, auto_ptr<InternalPluginEnvironment> &pluginEnv) {
	unsigned int i;
	for(i = 0; i < paths.size(); ++i) {
		try {
			fprintf(stderr, "Loading: %s\n", getFullName(paths[i], name).c_str());
			return tryLoad(getFullName(paths[i], name), pluginEnv);
		}
		catch(exception &e) {
			fprintf(stderr, "Failed to load plugin %s from path %s (%s)\n", name.c_str(), getFullName(paths[i], name).c_str(), e.what());
		}
	}
	throw runtime_error("Failed to load plugin %s");
}
