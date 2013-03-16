#include <vector>

#include "plugin.h"

using namespace std;

class CheckUnloadCallback : public pluginDestrCallback_t {
	public:
		typedef map<string, pluginHandle_t>::iterator StorageRef;
		inline CheckUnloadCallback(const StorageRef &storageRef) : mStorRef(storageRef) {}
		void onUnload();

	private:
		StorageRef mStorRef;
};

class LibraryHandleWithCallback : public LibraryHandle {
	public:
		inline LibraryHandleWithCallback(auto_ptr<CheckUnloadCallback> callback) : mCallback(callback) {};
		~LibraryHandleWithCallback();
		void onUnload();
	private:
		auto_ptr<CheckUnloadCallback> mCallback;

};

class pluginLoader_t {
	protected:
		vector<string> paths;

		string getFullName(const string &path, const string &name);
		auto_ptr<LibraryHandle> tryLoad(const string &path, const CheckUnloadCallback::StorageRef &storageRef);
	public:
//		pluginLoader_t();
		inline void addPath(const string &path) {
			paths.push_back(path);
		}

		auto_ptr<LibraryHandle> loadPlugin(const string &name, const CheckUnloadCallback::StorageRef &storageRef);
};
