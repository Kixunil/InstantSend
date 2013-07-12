#include <vector>

#include "plugin.h"

namespace InstantSend {

class LibraryHandleWithEnvironment : public LibraryHandle {
	public:
		inline LibraryHandleWithEnvironment(auto_ptr<InternalPluginEnvironment> &environment) : mEnvironment(environment) {};
		~LibraryHandleWithEnvironment();
		void onUnload();
	private:
		auto_ptr<InternalPluginEnvironment> mEnvironment;

};


class PluginLoader {
	protected:
		vector<std::string> paths;

		std::string getFullName(const std::string &path, const std::string &name);
		auto_ptr<LibraryHandle> tryLoad(const std::string &path, auto_ptr<InternalPluginEnvironment> &pluginEnv);
	public:
//		pluginLoader_t();
		inline void addPath(const std::string &path) {
			paths.push_back(path);
		}

		auto_ptr<LibraryHandle> loadPlugin(const std::string &name, auto_ptr<InternalPluginEnvironment> &pluginEnv);
};

}
