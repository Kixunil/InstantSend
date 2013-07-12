#include "pluginapi.h"

namespace InstantSend {

class InternalEventHandler : public EventPlugin {
	public:
		InternalEventHandler();
		~InternalEventHandler();
		void onLoad(const string &pluginName);
		void onUnload(const string &pluginName);
};

}
