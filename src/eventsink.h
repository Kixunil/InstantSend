#ifndef EVENTSINK
#define EVENTSINK

#include <set>

#include "pluginapi.h"
#include "plugin.h"
#include "multithread.h"

#define BCAST_EVENT(evType, evName, dataType) \
	inline void bcast ## evType ## evName (dataType &eventData) { \
		eventSink_t::instance().send ## evType ## evName (eventData); \
	}
/*
class eventData_t {
	public:
		virtual void sendEvent(event_t &event) = 0;
		virtual ~eventData_t();
};
*/

#define EVENTTYPE(evType) \
	private: \
		set<event ## evType ## _t *> evType ## Events; \
	public: \
		void reg ## evType (event ## evType ## _t &eventHandler)  throw(); \
		void unreg ## evType (event ## evType ## _t &eventHandler)  throw()

#define SENDEVDECL(evType, evName, dataType) \
	public: \
		virtual void send ## evType ## evName (dataType &eventData) throw()

class eventSink_t : public eventRegister_t {
	EVENTTYPE(Progress);
	EVENTTYPE(Connections);
	EVENTTYPE(Receive);
	EVENTTYPE(Send);
	EVENTTYPE(Server);
	EVENTTYPE(Plugin);

	SENDEVDECL(Progress, Begin, fileStatus_t);
	SENDEVDECL(Progress, Update, fileStatus_t);
	SENDEVDECL(Progress, Pause, fileStatus_t);
	SENDEVDECL(Progress, Resume, fileStatus_t);
	SENDEVDECL(Progress, End, fileStatus_t);

	SENDEVDECL(Server, Started, serverController_t);
	SENDEVDECL(Server, Stopped, serverController_t);

	SENDEVDECL(Plugin, Load, const string);
	SENDEVDECL(Plugin, Unload, const string);
	private:
		template <class TEvent, class TData> void sendEvent(set<TEvent *> &evSet, void (TEvent::*event)(TData &), TData &eventData) {
			MutexHolder mh(mMutex);
			for(typename set<TEvent *>::iterator it = evSet.begin(); it != evSet.end(); ++it) ((*it)->*event)(eventData);
		}

		map<string, PluginPtr<eventHandlerCreator_t> > eventPlugins;
		Mutex mMutex;
	public:
		static eventSink_t &instance();
		void autoLoad(jsonObj_t &plugins);
		void loadEventPlugin(const string &name, jsonComponent_t &config);
		void unloadEventPlugin(const string &name);
		void unloadAllEventPlugins();

};

BCAST_EVENT(Progress, Begin, fileStatus_t);
BCAST_EVENT(Progress, Update, fileStatus_t);
BCAST_EVENT(Progress, Pause, fileStatus_t);
BCAST_EVENT(Progress, Resume, fileStatus_t);
BCAST_EVENT(Progress, End, fileStatus_t);

BCAST_EVENT(Server, Started, serverController_t);
BCAST_EVENT(Server, Stopped, serverController_t);

BCAST_EVENT(Plugin, Load, const string);
BCAST_EVENT(Plugin, Unload, const string);

#endif
