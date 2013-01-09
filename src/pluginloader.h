#include <vector>

#include "plugin.h"

using namespace std;

class checkUnloadCallback_t : public pluginEmptyCallback_t {
	public:
		inline checkUnloadCallback_t(const string &pluginName) : name(pluginName) {}
		void empty();

	private:
		string name;
};

class pluginLoader_t {
	protected:
		vector<string> paths;

		string getFullName(const string &path, const string &name);
		void *tryLoad(const string &path);
		pluginInstanceCreator_t *getCreator(void *handle, const string &pluginName);
		void (*getPluginDestructor())(void *);
	public:
//		pluginLoader_t();
		inline void addPath(const string &path) {
			paths.push_back(path);
		}

		plugin_t loadPlugin(string name);
};
