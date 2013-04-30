#include "pluginapi.h"

class InternalEventHandler : public eventPlugin_t {
	public:
		InternalEventHandler();
		~InternalEventHandler();
		void onLoad(const string &pluginName);
		void onUnload(const string &pluginName);
};
