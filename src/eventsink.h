#include <set>

#include "pluginapi.h"

#define BCAST_SIMPLE_EVENT(eventName, eventCall) \
	class eventName :public eventData_t {\
		private:\
			fileStatus_t *fstatus;\
		public:\
			inline eventName(fileStatus_t &fileStatus) {\
				fstatus = &fileStatus;\
				eventSink_t::instance().eventCall(*this);\
			}\
			void sendEvent(event_t &event);\
	};

class eventData_t {
	public:
		virtual void sendEvent(event_t &event) = 0;
		virtual ~eventData_t();
};

class eventSink_t : public eventRegister_t {
	private:
		set<event_t *> progressEvents;
		set<event_t *> connectionEvents;
		set<event_t *> recvEvents;
		set<event_t *> sendEvents;


	public:
		void sendEvent(set<event_t *> &handlers, eventData_t &eventData);
		static eventSink_t &instance();
		void regProgress(eventProgress_t &progressEvent);
		void regConnections(eventConnections_t &connectionEvent);
		void regReceive(eventReceive_t &receiveEvent);
		void regSend(eventSend_t &sendEvent);

		inline void sendProgress(eventData_t &eventData) {
			sendEvent(progressEvents, eventData);
		}
		inline void sendConnection(eventData_t &eventData) {
			sendEvent(connectionEvents, eventData);
		}

		void autoLoad(jsonObj_t &plugins);

};

BCAST_SIMPLE_EVENT(bcastProgressBegin, sendProgress);
BCAST_SIMPLE_EVENT(bcastProgressUpdated, sendProgress);
BCAST_SIMPLE_EVENT(bcastProgressPaused, sendProgress);
BCAST_SIMPLE_EVENT(bcastProgressResumed, sendProgress);
BCAST_SIMPLE_EVENT(bcastProgressEnded, sendProgress);
