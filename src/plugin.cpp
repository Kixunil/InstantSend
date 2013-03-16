#include "plugin.h"
#include <cstdio>

LibraryHandle::~LibraryHandle() {}

pluginHandle_t::pluginHandle_t(auto_ptr<LibraryHandle> library) : refCnt(0), mLibrary(library.release()), mOwner(true) {
}

pluginHandle_t::pluginHandle_t(const pluginHandle_t &other) : refCnt(0), mLibrary(other.mLibrary), mOwner(false) {
}

pluginHandle_t::~pluginHandle_t() {
	fprintf(stderr, "~pluginHandle_t(); mOwner = %d\n", (int)mOwner);
	if(mOwner) delete mLibrary;
}

/*
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

*/
