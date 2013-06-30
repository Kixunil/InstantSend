#include <cstring>
#include <stdexcept>
#include <cstdio>

#include "datareceiver.h"
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

void dataReceiver_t::parseHeader(jsonObj_t &h) {
	if(dynamic_cast<jsonStr_t &>(h["service"]) != "filetransfer") throw runtime_error("Service not supported");
	string fname(dynamic_cast<jsonStr_t &>(h.gie("filename")).getVal());
	File::Size fsize = dynamic_cast<jsonInt_t &>(h.gie("filesize")).getVal();

	//for(unsigned int i = 0; i < fname.size(); ++i) if(fname[i] == '/') fname[i] = '-'; //strip slashes TODO: use OS independent function
	// stripPath(fname);

	if(pathIsUnsafe(fname)) {
		auto_ptr<anyData> data(allocData(75));
		strcpy(data->data, "{\"service\":\"filetransfer\",\"action\":\"reject\",\"reason\":\"Filename is unsafe\"}");
		cptr->sendData(data.get());
		return;
	}
	string destPath(combinePath(savedir, fname));
	makePath(destPath);

#ifdef DEBUG
	fprintf(stderr, "Receiving file %s (%zu bytes)\n", fname.c_str(), (size_t)fsize);
	fflush(stderr);
#endif

	fileWriter_t *writer = dynamic_cast<fileWriter_t *>(&fileList_t::getList().getController(0, destPath, (File::Size)fsize, cptr->getMachineIdentifier()));
	writer->incRC();
	D("Writer created");

	auto_ptr<anyData> data(allocData(53));
	strcpy(data->data, "{ \"service\" : \"filetransfer\", \"action\" : \"accept\" }");
	data->size = 53;
	cptr->sendData(data.get());

	receiveFileData(*writer);
}

void dataReceiver_t::receiveFileData(fileWriter_t &writer) {
	auto_ptr<anyData> data(allocData(DMAXSIZE + 1));
	while((data->size = DMAXSIZE) && cptr->recvData(data.get())) {
		data->data[data->size] = 0;
		string header(string(data->data));
		//fprintf(stderr, "Header: %s\n", header.c_str());
		size_t hlen = strlen(data->data);
		try {
			jsonObj_t h = jsonObj_t(&header);

			try {
				File::Size position = dynamic_cast<jsonInt_t &>(h.gie("position")).getVal();
				auto_ptr<anyData> rawData = allocData(data->size - hlen - 1);
				memcpy(rawData->data, data->data + hlen + 1, data->size - hlen - 1);
				rawData->size = data->size - hlen - 1;
				writer.writeData(position, rawData);
			}
			catch(jsonNotExist &e) {
				if(dynamic_cast<jsonStr_t &>(h["action"]).getVal() == "finished") {
					writer.decRC();
					return;
				} else throw;
			}
		}
		catch(exception &e) {
			sendErrMsg(cptr, e.what());
			writer.decRC();
			fprintf(stderr, "Exception while receiving: %s\n", e.what());
			return;
		}

		/*fprintf(stderr, "Received %u bytes of data.\n", data->size);
		fflush(stderr);*/
	}
	D("Connection terminated");
	writer.decRC();
}

void dataReceiver_t::run() {
	D("Client thread started");
	auto_ptr<anyData> data = allocData(DMAXSIZE + 1);
	while((data->size = DMAXSIZE) && cptr->recvData(data.get())) {
		data->data[data->size] = 0; // make sure it won't overflow
		string header(data->data); // extract header
		//fprintf(stderr, "Header: %s\n", header.c_str());
		try {
			jsonObj_t h(&header);
			parseHeader(h);
		}	
		catch(const exception &e) {
			puts(e.what());
			// reply with information about error
			jsonObj_t msgobj = jsonObj_t("{ \"service\" : \"filetransfer\", \"action\" : \"error\" }");
			jsonStr_t tmp;
			tmp.setVal(e.what());
			msgobj.insertVal("reason", &tmp);
			sendHeader(msgobj);
			return;
		}
	}

	return;
}

void dataReceiver_t::sendHeader(jsonComponent_t &json) {
	string msg(json.toString());
	auto_ptr<anyData> msgdata = allocData(msg.size() + 1);
	cptr->sendData(msgdata.get());
}

bool dataReceiver_t::autoDelete() {
	return true;
}
