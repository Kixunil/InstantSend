#include "internalevents.h"
#include "appcontrol.h"
#include "eventsink.h"

InternalEventHandler::InternalEventHandler() {
	eventSink_t::instance().regPlugin(*this);
}

void InternalEventHandler::onLoad(const string &pluginName) {
	(void)pluginName;
}

void InternalEventHandler::onUnload(const string &pluginName) {
	(void)pluginName;
	unfreezeMainThread();
}

InternalEventHandler::~InternalEventHandler() {
	eventSink_t::instance().unregPlugin(*this);
}
