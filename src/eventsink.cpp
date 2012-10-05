#include "eventsink.h"
#include "pluginlist.h"

#define BCAST_SIMPLE_EVENT_DEF(eventName, eventType, eventCall) \
void eventName::sendEvent(event_t &event) {\
	dynamic_cast<eventType &>(event).eventCall(*fstatus);\
}

event_t::~event_t() {}

eventData_t::~eventData_t() {}

eventSink_t &eventSink_t::instance() {
	static auto_ptr<eventSink_t> sink(new eventSink_t());
	return *sink.get();
}

void eventSink_t::regProgress(eventProgress_t &progressEvent) {
	progressEvents.push_back(&progressEvent);
}

void eventSink_t::regConnections(eventConnections_t &connectionEvent) {
	connectionEvents.push_back(&connectionEvent);
}

void eventSink_t::regReceive(eventReceive_t &receiveEvent) {
	recvEvents.push_back(&receiveEvent);
}

void eventSink_t::regSend(eventSend_t &sendEvent) {
	sendEvents.push_back(&sendEvent);
}

void eventSink_t::autoLoad(jsonObj_t &plugins) {
	pluginList_t &pl = pluginList_t::instance();
	for(jsonIterator it = plugins.begin(); it != plugins.end(); ++it) {
		try {
			dynamic_cast<eventHandlerCreator_t &>(*pl[it.key()].creator()).regEvents(*this, it.value());
		}
		catch(exception &e) {
			printf("Can't load event handler plugin '%s': %s\n", it.key().c_str(), e.what());
		}
	}
}

void eventSink_t::sendEvent(vector<event_t *> &handlers, eventData_t &eventData) {
	for(unsigned int i = 0; i < handlers.size(); ++i) {
		eventData.sendEvent(*handlers[i]);
	}
}

BCAST_SIMPLE_EVENT_DEF(bcastProgressBegin, eventProgress_t, onBegin);
BCAST_SIMPLE_EVENT_DEF(bcastProgressUpdated, eventProgress_t, onUpdate);
BCAST_SIMPLE_EVENT_DEF(bcastProgressPaused, eventProgress_t, onPause);
BCAST_SIMPLE_EVENT_DEF(bcastProgressResumed, eventProgress_t, onResume);
BCAST_SIMPLE_EVENT_DEF(bcastProgressEnded, eventProgress_t, onEnd);

