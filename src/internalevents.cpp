#include "internalevents.h"
#include "eventsink.h"

using namespace InstantSend;
using namespace std;

InternalEventHandler::InternalEventHandler() {
	EventSink::instance().regPlugin(*this);
}

void InternalEventHandler::onLoad(const string &pluginName) {
	(void)pluginName;
}

void InternalEventHandler::onUnload(const string &pluginName) {
	(void)pluginName;
	unfreezeMainThread();
}

InternalEventHandler::~InternalEventHandler() {
	EventSink::instance().unregPlugin(*this);
}
