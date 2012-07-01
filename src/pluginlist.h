#include <map>
#include <string>

#include "pluginapi.h"
#include "pluginloader.h"

using namespace std;

class pluginList_t { // Singleton DP
	private:
		map<string, plugin_t> storage;
		pluginLoader_t loader;
	public:
		plugin_t &operator[](const string &name);
		inline plugin_t &operator[](const char *name) {
			return (*this)[string(name)];
		}
		static pluginList_t *instance();
		inline void addSearchPath(const string &path) {
			loader.addPath(path);
		}
};

