#include <vector>

#include "plugin.h"

namespace InstantSend {

class LibraryHandleWithEnvironment : public LibraryHandle {
	public:
		inline LibraryHandleWithEnvironment(std::auto_ptr<InternalPluginEnvironment> &environment) : mEnvironment(environment) {};
		~LibraryHandleWithEnvironment();
		void onUnload();
	private:
		std::auto_ptr<InternalPluginEnvironment> mEnvironment;

};


class PluginLoader {
	protected:
		std::vector<std::string> paths;

		std::string getFullName(const std::string &path, const std::string &name);
		std::auto_ptr<LibraryHandle> tryLoad(const std::string &path, std::auto_ptr<InternalPluginEnvironment> &pluginEnv);
	public:
//		pluginLoader_t();
		inline void addPath(const std::string &path) {
			paths.push_back(path);
		}

		std::auto_ptr<LibraryHandle> loadPlugin(const std::string &name, std::auto_ptr<InternalPluginEnvironment> &pluginEnv);
};

}
