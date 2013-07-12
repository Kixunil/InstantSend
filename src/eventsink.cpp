#include <stdio.h>
#include <stdexcept>

#include "pluginlist.h"
#include "eventsink.h"

#define BCAST_EVENT_DEF(evType, evName, dataType) \
void EventSink::send ## evType ## evName (dataType &eventData) throw() {\
	sendEvent< Event ## evType, dataType >(evType ## Events, &Event ## evType::on ## evName, eventData); \
}

#define EVENTDEF(evType) \
	void EventSink::reg ## evType (Event ## evType &eventHandler) throw() { \
		MutexHolder mh(mMutex); \
		evType ## Events.insert(&eventHandler); \
	} \
	void EventSink::unreg ## evType (Event ## evType &eventHandler) throw() { \
		MutexHolder mh(mMutex); \
		evType ## Events.erase(&eventHandler); \
	}

using namespace InstantSend;
using namespace std;

EventSink &EventSink::instance() {
	static EventSink sink;
	return sink;
}

EVENTDEF(Progress);
EVENTDEF(Connections);
/*
EVENTDEF(Receive);
EVENTDEF(Send);
*/
EVENTDEF(Server);
EVENTDEF(Plugin);

void EventSink::autoLoad(jsonObj_t &plugins) {
	for(jsonIterator it = plugins.begin(); it != plugins.end(); ++it) {
		try {
			loadEventPlugin(it.key(), it.value());
		}
		catch(exception &e) {
			fprintf(stderr, "Can't load event handler plugin '%s': %s\n", it.key().c_str(), e.what());
		}
	}
}

void EventSink::loadEventPlugin(const string &name, jsonComponent_t &config) {
	if(eventPlugins.count(name)) throw runtime_error(string("Plugin ") + name + " already loaded");
	PluginPtr<EventHandlerCreator> plugin(PluginList::instance()[name].as<EventHandlerCreator>());
	plugin->regEvents(*this, &config);
	eventPlugins[name] = plugin;
}

void EventSink::unloadEventPlugin(const string &name) {
	map<string, PluginPtr<EventHandlerCreator> >::iterator plugin(eventPlugins.find(name));
	if(plugin == eventPlugins.end()) throw runtime_error(string("Plugin ") + name + " not loaded");
	plugin->second->unregEvents(*this);
	eventPlugins.erase(plugin);
	//pluginList_t::instance().checkUnload(name);
}

void EventSink::unloadAllEventPlugins() {
	while(eventPlugins.size()) {
		map<string, PluginPtr<EventHandlerCreator> >::iterator it(eventPlugins.begin());
		string pluginName(it->first);
		fprintf(stderr, "Unloading: %s\n", pluginName.c_str());
		it->second->unregEvents(*this);
		eventPlugins.erase(it);
		//pl.checkUnload(pluginName);
		fprintf(stderr, "Unloaded.\n");
	}
}

BCAST_EVENT_DEF(Progress, Begin, FileStatus);
BCAST_EVENT_DEF(Progress, Update, FileStatus);
BCAST_EVENT_DEF(Progress, Pause, FileStatus);
BCAST_EVENT_DEF(Progress, Resume, FileStatus);
BCAST_EVENT_DEF(Progress, End, FileStatus);

BCAST_EVENT_DEF(Server, Started, ServerController);
BCAST_EVENT_DEF(Server, Stopped, ServerController);

BCAST_EVENT_DEF(Plugin, Load, const string);
BCAST_EVENT_DEF(Plugin, Unload, const string);
