#include "pluginapi.h"

namespace InstantSend {

class InternalEventHandler : public EventPlugin {
	public:
		InternalEventHandler();
		~InternalEventHandler();
		void onLoad(const std::string &pluginName);
		void onUnload(const std::string &pluginName);
};

}
