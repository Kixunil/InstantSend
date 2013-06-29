#ifndef DATARECEIVER
#define DATARECEIVER
#include "multithread.h"
#include "filewriter.h"
#include "plugin.h"

extern string savedir;

class dataReceiver_t: public thread_t {
	pluginInstanceAutoPtr<peer_t> cptr;
	public:
		inline dataReceiver_t(pluginInstanceAutoPtr<peer_t> &peer) : cptr(peer) {
		}

		void run(); 	// This is called as new thread
		bool autoDelete();
	protected:
		void parseHeader(jsonObj_t &h);
		void receiveFileData(fileWriter_t &writer);
		void sendHeader(jsonComponent_t &json);
};

#endif
