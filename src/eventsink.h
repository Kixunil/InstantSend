#ifndef EVENTSINK
#define EVENTSINK

#include <set>

#include "pluginapi.h"
#include "plugin.h"
#include "multithread.h"

#define BCAST_EVENT(evType, evName, dataType) \
	inline void bcast ## evType ## evName (dataType &eventData) { \
		EventSink::instance().send ## evType ## evName (eventData); \
	}
#define EVENTTYPE(evType) \
	private: \
		set<Event ## evType *> evType ## Events; \
	public: \
		void reg ## evType (Event ## evType &eventHandler)  throw(); \
		void unreg ## evType (Event ## evType &eventHandler)  throw()

#define SENDEVDECL(evType, evName, dataType) \
	public: \
		virtual void send ## evType ## evName (dataType &eventData) throw()

namespace InstantSend {

class EventSink : public EventRegister {
	EVENTTYPE(Progress);
	EVENTTYPE(Connections);
#if 0
	EVENTTYPE(Receive);
	EVENTTYPE(Send);
#endif
	EVENTTYPE(Server);
	EVENTTYPE(Plugin);

	SENDEVDECL(Progress, Begin, FileStatus);
	SENDEVDECL(Progress, Update, FileStatus);
	SENDEVDECL(Progress, Pause, FileStatus);
	SENDEVDECL(Progress, Resume, FileStatus);
	SENDEVDECL(Progress, End, FileStatus);

	SENDEVDECL(Server, Started, ServerController);
	SENDEVDECL(Server, Stopped, ServerController);

	SENDEVDECL(Plugin, Load, const string);
	SENDEVDECL(Plugin, Unload, const string);
	private:
		template <class TEvent, class TData> void sendEvent(set<TEvent *> &evSet, void (TEvent::*event)(TData &), TData &eventData) {
			MutexHolder mh(mMutex);
			for(typename set<TEvent *>::iterator it = evSet.begin(); it != evSet.end(); ++it) ((*it)->*event)(eventData);
		}

		map<string, PluginPtr<EventHandlerCreator> > eventPlugins;
		Mutex mMutex;
	public:
		static EventSink &instance();
		void autoLoad(jsonObj_t &plugins);
		void loadEventPlugin(const string &name, jsonComponent_t &config);
		void unloadEventPlugin(const string &name);
		void unloadAllEventPlugins();

};

BCAST_EVENT(Progress, Begin, FileStatus);
BCAST_EVENT(Progress, Update, FileStatus);
BCAST_EVENT(Progress, Pause, FileStatus);
BCAST_EVENT(Progress, Resume, FileStatus);
BCAST_EVENT(Progress, End, FileStatus);

BCAST_EVENT(Server, Started, ServerController);
BCAST_EVENT(Server, Stopped, ServerController);

BCAST_EVENT(Plugin, Load, const string);
BCAST_EVENT(Plugin, Unload, const string);

}

#endif
