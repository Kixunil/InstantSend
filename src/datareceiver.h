#include "multithread.h"
#include "pluginapi.h"

extern string savedir;

class dataReceiver_t: public thread_t {
	auto_ptr<peer_t> cptr;
	public:
		inline dataReceiver_t(auto_ptr<peer_t> peer) : cptr(peer) {
		}

		inline dataReceiver_t(peer_t *peer) : cptr(peer) {
		}

		void run(); 	// This is called as new thread
		bool autoDelete();
};


