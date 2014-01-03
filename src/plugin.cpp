#include "plugin.h"

using namespace std;
using namespace InstantSend;

InternalPluginEnvironment::InternalPluginEnvironment(Application &app, const std::string &name) :
	PluginEnvironment(app),
	mName(name),
	instanceCount(0),
	mSecureStorage(app.secureStorage()?
			new SecureStorageWrapper(string("plugin_") + name + "_", *app.secureStorage()):
			NULL) {
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
	LOG(Logger::VerboseDebug, "Created instance of plugin \"%s\".", mName.c_str());
	mRCMutex.lock();
	++instanceCount;
	mRCMutex.unlock();
}

void InternalPluginEnvironment::onInstanceDestroyed() {
	LOG(Logger::VerboseDebug, "Destroyed instance of plugin \"%s\".", mName.c_str());
	mRCMutex.lock();
	--instanceCount;
	if(!instanceCount) LOG(Logger::VerboseDebug, "Instance count of plugin \"%s\" reached zero.", mName.c_str());
	mRCMutex.unlock();
}

void InternalPluginEnvironment::log(Logger::Level level, const std::string &message) {
	instantSend->logger().log(level, message, mName);
}

void InternalPluginEnvironment::checkUnload() {
	LOG(Logger::VerboseDebug, "InternalPluginEnvironment::checkUnload(%p)", this);
	if(!instanceCount && mStorageHandle.get()) {
		LOG(Logger::VerboseDebug, "mStorageHandle->checkUnload();");
		mStorageHandle->checkUnload();
	}
}

SecureStorage *InternalPluginEnvironment::secureStorage() {
	return mSecureStorage.get();
}

LibraryHandle::~LibraryHandle() {}

pluginHandle_t::pluginHandle_t(auto_ptr<LibraryHandle> library) : refCnt(0), mLibrary(library.release()), mOwner(true) {
	LOG(Logger::VerboseDebug, "pluginHandle_t(%p, ...) called", this);
}

pluginHandle_t::pluginHandle_t(const pluginHandle_t &other) : refCnt(0), mLibrary(other.mLibrary), mOwner(false) {
	LOG(Logger::VerboseDebug, "pluginHandle_t(%p, ...) called", this);
}

pluginHandle_t::~pluginHandle_t() {
	if(mOwner) delete mLibrary;
	LOG(Logger::VerboseDebug, "~pluginHandle_t(%p) called; Owner = %d", this, (int)mOwner);
}

InstantSend::PluginStorageHandle::~PluginStorageHandle() {}
