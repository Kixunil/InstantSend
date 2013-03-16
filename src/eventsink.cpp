#include <stdio.h>
#include <stdexcept>

#include "eventsink.h"
#include "pluginlist.h"

#define BCAST_EVENT_DEF(evType, evName, dataType) \
void eventSink_t::send ## evType ## evName (dataType &eventData) throw() {\
	sendEvent< event ## evType ## _t, dataType >(evType ## Events, &event ## evType ## _t::on ## evName, eventData); \
}

#define EVENTDEF(evType) \
	void eventSink_t::reg ## evType (event ## evType ## _t &eventHandler) throw() { \
		mMutex->get(); \
		evType ## Events.insert(&eventHandler); \
		mMutex->release(); \
	} \
	void eventSink_t::unreg ## evType (event ## evType ## _t &eventHandler) throw() { \
		mMutex->get(); \
		evType ## Events.erase(&eventHandler); \
		mMutex->release(); \
	}

//eventData_t::~eventData_t() {}

eventSink_t &eventSink_t::instance() {
	static eventSink_t sink;
	return sink;
}

EVENTDEF(Progress);
EVENTDEF(Connections);
EVENTDEF(Receive);
EVENTDEF(Send);
EVENTDEF(Server);
EVENTDEF(Plugin);

void eventSink_t::autoLoad(jsonObj_t &plugins) {
	for(jsonIterator it = plugins.begin(); it != plugins.end(); ++it) {
		try {
			loadEventPlugin(it.key(), it.value());
		}
		catch(exception &e) {
			fprintf(stderr, "Can't load event handler plugin '%s': %s\n", it.key().c_str(), e.what());
		}
	}
}

void eventSink_t::loadEventPlugin(const string &name, jsonComponent_t &config) {
	if(eventPlugins.count(name)) throw runtime_error(string("Plugin ") + name + " already loaded");
	PluginPtr<eventHandlerCreator_t> plugin(pluginList_t::instance()[name].as<eventHandlerCreator_t>());
	plugin->regEvents(*this, &config);
	eventPlugins[name] = plugin;
}

void eventSink_t::unloadEventPlugin(const string &name) {
	map<string, PluginPtr<eventHandlerCreator_t> >::iterator plugin(eventPlugins.find(name));
	if(plugin == eventPlugins.end()) throw runtime_error(string("Plugin ") + name + " not loaded");
	plugin->second->unregEvents(*this);
	eventPlugins.erase(plugin);
	//pluginList_t::instance().checkUnload(name);
}

void eventSink_t::unloadAllEventPlugins() {
	while(eventPlugins.size()) {
		map<string, PluginPtr<eventHandlerCreator_t> >::iterator it(eventPlugins.begin());
		string pluginName(it->first);
		fprintf(stderr, "Unloading: %s\n", pluginName.c_str());
		it->second->unregEvents(*this);
		eventPlugins.erase(it);
		//pl.checkUnload(pluginName);
		fprintf(stderr, "Unloaded.\n");
	}
}

BCAST_EVENT_DEF(Progress, Begin, fileStatus_t);
BCAST_EVENT_DEF(Progress, Update, fileStatus_t);
BCAST_EVENT_DEF(Progress, Pause, fileStatus_t);
BCAST_EVENT_DEF(Progress, Resume, fileStatus_t);
BCAST_EVENT_DEF(Progress, End, fileStatus_t);

BCAST_EVENT_DEF(Server, Started, serverController_t);
BCAST_EVENT_DEF(Server, Stopped, serverController_t);

BCAST_EVENT_DEF(Plugin, Load, const string);
BCAST_EVENT_DEF(Plugin, Unload, const string);
