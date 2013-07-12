#include "plugin.h"
#include <cstdio>
#include "appcontrol.h"

using namespace std;
using namespace InstantSend;

InternalPluginEnvironment::InternalPluginEnvironment(ApplicationEnvironment &appEnv, const std::string &name) : PluginEnvironment(appEnv), mName(name) {
}

const std::string &InternalPluginEnvironment::systemPluginDataDir() {
	throw runtime_error("Unimplemented method");
}

const std::string &InternalPluginEnvironment::systemPluginConfigDir() {
	throw runtime_error("Unimplemented method");
}

const std::string &InternalPluginEnvironment::userPluginDir() {
	throw runtime_error("Unimplemented method");
}

void InternalPluginEnvironment::onInstanceCreated() {
	++instanceCount;
}

void InternalPluginEnvironment::onInstanceDestroyed() {
	--instanceCount;
}

void InternalPluginEnvironment::log(Logger::Level level, const std::string &message) {
	instantSend->logger().log(level, message, mName);
}

void InternalPluginEnvironment::checkUnload() {
}

LibraryHandle::~LibraryHandle() {}

pluginHandle_t::pluginHandle_t(auto_ptr<LibraryHandle> library) : refCnt(0), mLibrary(library.release()), mOwner(true) {
}

pluginHandle_t::pluginHandle_t(const pluginHandle_t &other) : refCnt(0), mLibrary(other.mLibrary), mOwner(false) {
}

pluginHandle_t::~pluginHandle_t() {
	if(mOwner) delete mLibrary;
}
