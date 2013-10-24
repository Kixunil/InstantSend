#ifndef DATARECEIVER
#define DATARECEIVER
#include "multithread.h"
#include "filewriter.h"
#include "plugin.h"

namespace InstantSend {

extern std::string savedir;

class DataReceiver: public thread_t {
	pluginInstanceAutoPtr<Peer> cptr;
	public:
		inline DataReceiver(pluginInstanceAutoPtr<Peer> &peer) : cptr(peer) {
		}

		void run(); 	// This is called as new thread
		bool autoDelete();
	protected:
		void parseHeader(jsonObj_t &h);
		void receiveFileData(fileWriter_t &writer, int protocolVersion);
		void sendHeader(jsonComponent_t &json);
};

}

#endif
