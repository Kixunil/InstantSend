#include <dlfcn.h>
#include <stdexcept>
#include <stdio.h>

#include "pluginloader.h"

class PosixLibraryHandle : public LibraryHandleWithCallback {
	public:
		inline PosixLibraryHandle(void *libHandle, auto_ptr<CheckUnloadCallback> callback) : LibraryHandleWithCallback(callback), mLibHandle(libHandle) {}

		~PosixLibraryHandle() {
//#ifndef NODLCLOSE
			/*void *data;
			if(dlinfo(mLibHandle, RTLD_DI_TLS_DATA, (void **)&data) < 0) {
				fprintf(stderr, "dlinfo: %s\n", dlerror());
			} else free(data); //hack*/

			dlclose(mLibHandle);
			fprintf(stderr, "Library closed\n");
//#endif
		}

		inline void setCreator(pluginInstanceCreator_t *creator) {
			mCreator = creator;
		}
	private:
		void *mLibHandle;
};

string pluginLoader_t::getFullName(const string &path, const string &name) {
	return path + "/" + name + LIBRARY_EXTENSION;
}

pluginInstanceCreator_t *getCreator(void *handle, pluginDestrCallback_t &callback) {
	pluginInstanceCreator_t *(* creator)(pluginDestrCallback_t &);
	*(void **)(&creator) = dlsym(handle, "getCreator");
	if(!creator) throw runtime_error("Invalid plugin");
	pluginInstanceCreator_t *result = creator(callback);
	return result;
}


auto_ptr<LibraryHandle> pluginLoader_t::tryLoad(const string &path, const CheckUnloadCallback::StorageRef &storageRef) {
	void *dlhandle = dlopen(path.c_str(), RTLD_LAZY);
	if(!dlhandle) throw runtime_error(dlerror());
	auto_ptr<CheckUnloadCallback> cb(new CheckUnloadCallback(storageRef));
	CheckUnloadCallback &cbRef(*cb);
	auto_ptr<PosixLibraryHandle> posixLib(new PosixLibraryHandle(dlhandle, cb));
	posixLib->setCreator(getCreator(dlhandle, cbRef));
	return auto_ptr<LibraryHandle>(posixLib);
}
