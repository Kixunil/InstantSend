#include "pluginapi.h"

pluginInstance_t::~pluginInstance_t() {
	instcreator.dec();
}

pluginInstanceCreator_t::~pluginInstanceCreator_t() {
}

#if 0
event_t::~event_t() {
}
#endif
