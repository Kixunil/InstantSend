#include <cstring>
#include <stdexcept>
#include <cstdio>

#include "datareceiver.h"
#include "filewriter.h"
#include "eventsink.h"
#include "sysapi.h"

//#define DEBUG
#ifdef DEBUG
#define D(MSG) fprintf(stderr, MSG "\n"); fflush(stderr);
#else
#define D(MSG) do ; while(0)
#endif

void sendErrMsg(const pluginInstanceAutoPtr<peer_t> &client, const char *msg) {
	// Sends information about error to client
	fprintf(stderr, "%s\n", msg);
	fflush(stderr);
	jsonObj_t msgobj = jsonObj_t("{ \"action\" : \"error\" }"); // Little bit lazy method
	jsonStr_t tmp(msg);
	msgobj.insertVal("message", &tmp);
	string retmsg = msgobj.toString();
	auto_ptr<anyData> data(allocData(retmsg.size() + 1));
	strcpy(data->data, retmsg.c_str());
	data->size = retmsg.size() + 1;
	client->sendData(data.get());
}

void dataReceiver_t::run() {
	D("Client thread started");
	auto_ptr<anyData> data = allocData(DMAXSIZE + 1);
	int received, hlen;
	string fname;
	fileList_t &flist = fileList_t::getList();
	fileWriter_t *writer = NULL;

	data->size = DMAXSIZE;
	received = cptr->recvData(data.get());
	uint32_t fsize;
	if(received) {
		data->data[data->size] = 0; // make sure it won't overflow
		string header(data->data); // extract header
		try {
			jsonObj_t h(&header);
			if(dynamic_cast<jsonStr_t &>(h.gie("service")) != "filetransfer") throw runtime_error("Service not supported");
			fname = dynamic_cast<jsonStr_t &>(h.gie("filename")).getVal();
			fsize = dynamic_cast<jsonInt_t &>(h.gie("filesize")).getVal();

			for(unsigned int i = 0; i < fname.size(); ++i) if(fname[i] == '/') fname[i] = '-'; //strip slashes TODO: use OS independent function
#ifdef DEBUG
			fprintf(stderr, "Receiving file %s (%d bytes)\n", fname.c_str(), fsize);
			fflush(stderr);
#endif

			writer = dynamic_cast<fileWriter_t *>(&flist.getController(0, combinePath(savedir, fname), (size_t)fsize, cptr->getMachineIdentifier()));
			writer->incRC();
			D("Writer created");

			strcpy(data->data, "{ \"service\" : \"filetransfer\", \"action\" : \"accept\" }");
			data->size = 52;
			cptr->sendData(data.get());
		}	
		catch(const exception &e) {
			if(writer) writer->decRC();
			puts(e.what());
			// reply with information about error
			jsonObj_t msgobj = jsonObj_t("{ \"service\" : \"filetransfer\", \"action\" : \"reject\" }");
			jsonStr_t tmp;
			tmp.setVal(e.what());
			msgobj.insertVal("reason", &tmp);
			string retmsg = msgobj.toString();
			strcpy(data->data, retmsg.c_str());
			data->size = retmsg.size() + 1;
			cptr->sendData(data.get());
			return;
		}
	} else {
		fprintf(stderr, "No data received - connection problem?\n");
		return;
	}

	int fileUncompleted = 1;

	D("Receiving data");
	do {
		string header;
		data->size = DMAXSIZE;
		received = cptr->recvData(data.get());
		if(received) {
			data->data[data->size] = 0;
			header = string(data->data);
			hlen = strlen(data->data);
			try {
				jsonObj_t h = jsonObj_t(&header);

				long position = dynamic_cast<jsonInt_t &>(h.gie("position")).getVal();
				auto_ptr<anyData> rawData = allocData(data->size - hlen - 1);
				memcpy(rawData->data, data->data + hlen + 1, data->size - hlen - 1);
				rawData->size = data->size - hlen - 1;
				while(!writer->writeData(position, rawData, *this)) pausePoint();
			}
			catch(const char *msg) {
				sendErrMsg(cptr, msg);
				//return;
			}
			catch(exception &e) {
				sendErrMsg(cptr, e.what());
			}

			/*fprintf(stderr, "Received %u bytes of data.\n", data->size);
			fflush(stderr);*/
		} else fprintf(stderr, "No data or error.\n");
	} while(received && fileUncompleted);

	fprintf(stderr, "Receiving finished.\n");
	//TODO make event
	writer->decRC();
	return;
}

bool dataReceiver_t::autoDelete() {
	return true;
}
