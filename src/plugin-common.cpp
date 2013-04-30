#include "pluginapi.h"

pluginInstance_t::~pluginInstance_t() {
	instcreator.dec();
}

pluginInstanceCreator_t::~pluginInstanceCreator_t() {
}

bool pluginInstanceCreator_t::unloadPossible() {
	return true;
}

bool pluginMultiInstanceCreator_t::unloadPossible() {
	return !count;
}

pluginMultiInstanceCreator_t::~pluginMultiInstanceCreator_t() {
}

#if 0
event_t::~event_t() {
}
#endif
