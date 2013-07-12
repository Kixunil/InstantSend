#include <windows.h>
#include <stdexcept>
#include <stdio.h>

#include "pluginloader.h"

using namespace InstantSend;
using namespace std;

class WindowsLibraryHandle : public LibraryHandleWithEnvironment {
	public:
		inline WindowsLibraryHandle(HMODULE libHandle, auto_ptr<InternalPluginEnvironment> env) : LibraryHandleWithEnvironment(env), mLibHandle(libHandle) {};

		~WindowsLibraryHandle() {
			FreeLibrary(mLibHandle);
			fprintf(stderr, "Library closed\n");
		}

		inline void setCreator(PluginInstanceCreator *creator) {
			mCreator = creator;
		}
	private:
		HMODULE mLibHandle;
};

string PluginLoader::getFullName(const string &path, const string &name) {
	return path + "\\" + name + LIBRARY_EXTENSION;
}

typedef PluginInstanceCreator *(*getCreatorFunc)(PluginEnvironment &);

PluginInstanceCreator *getCreator(HMODULE handle, PluginEnvironment &env) {
	FARPROC gc = GetProcAddress(handle, "getCreator");
	if(!gc) throw runtime_error("Invalid plugin");
	getCreatorFunc creator = (getCreatorFunc)gc;
	return creator(env);
}

auto_ptr<LibraryHandle> PluginLoader::tryLoad(const string &path, auto_ptr<InternalPluginEnvironment> &pluginEnv) {
	// Open library
	HMODULE lib = LoadLibrary(path.c_str());
	if(!lib) throw runtime_error("LoadLibrary failed");

	// Get creator
	PluginInstanceCreator *creator = getCreator(lib, *pluginEnv);

	// Create library handle
	auto_ptr<WindowsLibraryHandle> windowsLib(new WindowsLibraryHandle(lib, pluginEnv));
	windowsLib->setCreator(creator);

	return auto_ptr<LibraryHandle>(windowsLib);                                   
}
