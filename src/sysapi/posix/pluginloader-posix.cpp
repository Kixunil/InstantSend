#include <dlfcn.h>
#include <stdexcept>
#include <stdio.h>

#include "pluginloader.h"

using namespace InstantSend;
using namespace std;

class PosixLibraryHandle : public LibraryHandleWithEnvironment {
	public:
		inline PosixLibraryHandle(void *libHandle, auto_ptr<InternalPluginEnvironment> &env) : LibraryHandleWithEnvironment(env), mLibHandle(libHandle) {}

		~PosixLibraryHandle() {
			dlclose(mLibHandle);
			fprintf(stderr, "Library closed\n");
		}

		inline void setCreator(PluginInstanceCreator *creator) {
			mCreator = creator;
		}
	private:
		void *mLibHandle;
};

string PluginLoader::getFullName(const string &path, const string &name) {
	return path + "/" + name + LIBRARY_EXTENSION;
}

PluginInstanceCreator *getCreator(void *handle, PluginEnvironment &env) {
	PluginInstanceCreator *(* creator)(PluginEnvironment &);
	*(void **)(&creator) = dlsym(handle, "getCreator");
	if(!creator) throw runtime_error("Invalid plugin");
	PluginInstanceCreator *result = creator(env);
	return result;
}


auto_ptr<LibraryHandle> PluginLoader::tryLoad(const string &path, auto_ptr<InternalPluginEnvironment> &pluginEnv) {
	// Open library
	void *dlhandle = dlopen(path.c_str(), RTLD_LAZY);
	if(!dlhandle) throw runtime_error(dlerror());

	// Get creator
	PluginInstanceCreator *creator = getCreator(dlhandle, *pluginEnv);

	// Create library handle
	auto_ptr<PosixLibraryHandle> posixLib(new PosixLibraryHandle(dlhandle, pluginEnv));
	posixLib->setCreator(creator);

	return auto_ptr<LibraryHandle>(posixLib);
}
