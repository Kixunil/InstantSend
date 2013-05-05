#include <windows.h>
#include <stdexcept>
#include <stdio.h>

#include "pluginloader.h"

class WindowsLibraryHandle : public LibraryHandleWithCallback {
	public:
		inline WindowsLibraryHandle(HMODULE libHandle, auto_ptr<CheckUnloadCallback> callback) : LibraryHandleWithCallback(callback), mLibHandle(libHandle) {};

		~WindowsLibraryHandle() {
			FreeLibrary(mLibHandle);
			fprintf(stderr, "Library closed\n");
		}

		inline void setCreator(pluginInstanceCreator_t *creator) {
			mCreator = creator;
		}
	private:
		HMODULE mLibHandle;
};

string pluginLoader_t::getFullName(const string &path, const string &name) {
	return path + "\\" + name + LIBRARY_EXTENSION;
}

typedef pluginInstanceCreator_t *(*getCreatorFunc)(pluginDestrCallback_t &);

pluginInstanceCreator_t *getCreator(HMODULE handle, pluginDestrCallback_t &callback) {
	FARPROC gc = GetProcAddress(handle, "getCreator");
	if(!gc) throw runtime_error("Invalid plugin");
	getCreatorFunc creator = (getCreatorFunc)gc;
	return creator(callback);
}

auto_ptr<LibraryHandle> pluginLoader_t::tryLoad(const string &path, const CheckUnloadCallback::StorageRef &storageRef) {
	fprintf(stderr, "Loading: %s\n", path.c_str());
	HMODULE lib = LoadLibrary(path.c_str());
	if(!lib) throw runtime_error("LoadLibrary failed");
	auto_ptr<CheckUnloadCallback> cb(new CheckUnloadCallback(storageRef));
	CheckUnloadCallback &cbRef(*cb);
	auto_ptr<WindowsLibraryHandle> posixLib(new WindowsLibraryHandle(lib, cb));
	posixLib->setCreator(getCreator(lib, cbRef));                          
	return auto_ptr<LibraryHandle>(posixLib);                                   
}
