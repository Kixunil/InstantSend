#include <set>

#include "pluginapi.h"

using namespace std;

class pluginHandle_t {
	private:
		int refCnt;
		void *handle;
		pluginInstanceCreator_t *instCreator;
		void (*handleDestructor)(void *);
		set<pluginInstance_t *> instances;
	public:
		pluginHandle_t(void *handle, pluginInstanceCreator_t *pluginInstanceCreator, void (*handleDestructor)(void *));

		~pluginHandle_t();
		inline void incRC() {
			++refCnt;
		}

		inline int decRC() {
			return !--refCnt;
		}

		inline pluginInstanceCreator_t *creator() {
			return instCreator;
		}

		inline void addInstance(pluginInstance_t *instance) {
			instances.insert(instance);
		}

		inline void removeInstance(pluginInstance_t *instance) {
			instances.erase(instance);
		}

		inline bool loaded() {
			return instCreator && handle;
		}
};

class plugin_t {
	private:
		pluginHandle_t *handle;
	public:
		plugin_t(const pluginHandle_t *handle);
		plugin_t(const plugin_t &plugin);
		inline plugin_t() {
			handle = NULL;
		}
		~plugin_t();
		plugin_t &operator=(plugin_t &plugin);
		pluginHandle_t *operator=(pluginHandle_t *handle);

		inline pluginInstanceCreator_t *creator() {
			return handle->creator();
		}

		inline bool loaded() {
			return handle && handle->loaded();
		}

		inline peer_t * newClient(const jsonComponent_t &config) {
			return dynamic_cast<connectionCreator_t &>(*handle->creator()).newClient(config);
		}

		inline serverPlugin_t *newServer(const jsonComponent_t &config) {
			return dynamic_cast<connectionCreator_t &>(*handle->creator()).newServer(config);
		}

		inline const char *lastError() {
			return handle->creator()->getErr();
		}
};
