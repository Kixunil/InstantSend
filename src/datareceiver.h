#include "multithread.h"
#include "plugin.h"

extern string savedir;

class dataReceiver_t: public thread_t {
	pluginInstanceAutoPtr<peer_t> cptr;
	public:
		inline dataReceiver_t(pluginInstanceAutoPtr<peer_t> &peer) : cptr(peer) {
		}

		void run(); 	// This is called as new thread
		bool autoDelete();
};


