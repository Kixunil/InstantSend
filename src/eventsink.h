#include <set>

#include "pluginapi.h"

#define BCAST_SIMPLE_EVENT(eventName, eventCall, dataType) \
	class eventName :public eventData_t {\
		private:\
			dataType &evdata;\
		public:\
			inline eventName(dataType &data) : evdata(data) {\
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
		set<event_t *> serverEvents;

	public:
		void sendEvent(set<event_t *> &handlers, eventData_t &eventData);
		static eventSink_t &instance();
		void regProgress(eventProgress_t &progressEvent);
		void regConnections(eventConnections_t &connectionEvent);
		void regReceive(eventReceive_t &receiveEvent);
		void regSend(eventSend_t &sendEvent);
		void regServer(eventServer_t &serverEvent);

		void unregProgress(eventProgress_t &progressEvent);
		void unregConnections(eventConnections_t &connectionEvent);
		void unregReceive(eventReceive_t &receiveEvent);
		void unregSend(eventSend_t &sendEvent);
		void unregServer(eventServer_t &serverEvent);

		inline void sendProgress(eventData_t &eventData) {
			sendEvent(progressEvents, eventData);
		}

		inline void sendConnection(eventData_t &eventData) {
			sendEvent(connectionEvents, eventData);
		}

		inline void sendServer(eventData_t &eventData) {
			sendEvent(serverEvents, eventData);
		}

		void autoLoad(jsonObj_t &plugins);

};

BCAST_SIMPLE_EVENT(bcastProgressBegin, sendProgress, fileStatus_t);
BCAST_SIMPLE_EVENT(bcastProgressUpdated, sendProgress, fileStatus_t);
BCAST_SIMPLE_EVENT(bcastProgressPaused, sendProgress, fileStatus_t);
BCAST_SIMPLE_EVENT(bcastProgressResumed, sendProgress, fileStatus_t);
BCAST_SIMPLE_EVENT(bcastProgressEnded, sendProgress, fileStatus_t);

BCAST_SIMPLE_EVENT(bcastServerStarted, sendServer, serverController_t);
BCAST_SIMPLE_EVENT(bcastServerStopped, sendServer, serverController_t);
