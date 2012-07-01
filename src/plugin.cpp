#include "plugin.h"

pluginHandle_t::pluginHandle_t(void *handle, pluginInstanceCreator_t *pluginInstanceCreator, void (*handleDestructor)(void *)) {
	this->handle = handle;
	instCreator = pluginInstanceCreator;
	this->handleDestructor = handleDestructor;
	refCnt = 0;
}

pluginHandle_t::~pluginHandle_t() {
	set<pluginInstance_t *>::iterator b = instances.begin(), e = instances.end();

	for(; b != e; ++b) {
		delete *b;
	}

	delete instCreator;
	instCreator = NULL;
	handleDestructor(handle);
	handle = NULL;
}

plugin_t::plugin_t(const pluginHandle_t *handle) {
	this->handle = (pluginHandle_t *)handle;
	if(handle) this->handle->incRC();
}

plugin_t::plugin_t(const plugin_t &plugin) {
	if(&plugin == this) return;
	handle = plugin.handle;
	if(handle) handle->incRC();
}

plugin_t &plugin_t::operator=(plugin_t &plugin) {
	if(&plugin == this) return plugin;
	handle = plugin.handle;
	if(handle) handle->incRC();
	return *this;
}

pluginHandle_t *plugin_t::operator=(pluginHandle_t *handle) {
	if(this->handle == handle) return handle;
	if(this->handle && this->handle->decRC()) delete this->handle;
	this->handle = handle;
	if(handle) handle->incRC();
	return handle;
}

plugin_t::~plugin_t() {
	if(handle && handle->decRC()) {
		delete handle;
		handle = NULL;
	}
}

pluginInstance_t::~pluginInstance_t() {
	;
}
