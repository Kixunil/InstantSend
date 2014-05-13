#include <cstring>
#include <stdexcept>
#include <cstdio>

#include "datareceiver.h"
#include "eventsink.h"
#include "sysapi.h"

//#define DEBUG
#ifdef DEBUG
#define D(MSG) LOG(Logger::VerboseDebug, MSG)
#else
#define D(MSG) do ; while(0)
#endif

using namespace InstantSend;
using namespace std;

const int maxProtocolVersion = 1;

void sendErrMsg(const pluginInstanceAutoPtr<Peer> &client, const char *msg) {
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

void DataReceiver::parseHeader(jsonObj_t &h) {
	if(dynamic_cast<jsonStr_t &>(h["service"]) != "filetransfer") throw runtime_error("Service not supported");
	string fname(dynamic_cast<jsonStr_t &>(h.gie("filename")).getVal());
	File::Size fsize = dynamic_cast<jsonInt_t &>(h.gie("filesize")).getVal();
	int protocolVersion;
	jsonObj_t *extras;
	try {
		protocolVersion = dynamic_cast<jsonInt_t &>(h["version"]).getVal();
	}
	catch(...) {
		protocolVersion = 0;
	}

	if(protocolVersion > maxProtocolVersion) {
		protocolVersion = maxProtocolVersion;
	}

	try {
		extras = &dynamic_cast<jsonObj_t &>(h["extras"]);
	}
	catch(...) {
		extras = NULL;
	}

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

	LOG(Logger::Note, "Receiving file %s (%zu bytes)\n", fname.c_str(), (size_t)fsize);

	FileWriterPtr writer(dynamic_cast<fileWriter_t &>(fileList_t::getList().getController(0, destPath, (File::Size)fsize, cptr->getMachineIdentifier(), extras)));
	LOG(Logger::Debug, "Writer created");

	jsonObj_t response("{ \"service\" : \"filetransfer\", \"action\" : \"accept\" }");
	response.insertNew("version", new jsonInt_t(protocolVersion));
	response.insertNew("max_msg_size", new jsonInt_t(DMAXSIZE));
	LOG(Logger::VerboseDebug, "Response object crated, converting and sendng...");
	string responseMsg(response.toString());
	LOG(Logger::VerboseDebug, "Response: \"%s\"", responseMsg.c_str());
	auto_ptr<anyData> data(allocData(responseMsg.size() + 1)); // +1 for null terminator
	strcpy(data->data, responseMsg.c_str());
	data->size = responseMsg.size(); // no need to send null terminator
	cptr->sendData(data.get());

	LOG(Logger::VerboseDebug, "Receiving file data");
	receiveFileData(*writer, protocolVersion);
}

void DataReceiver::receiveFileData(fileWriter_t &writer, int protocolVersion) {
	auto_ptr<anyData> data(allocData(DMAXSIZE + 1));
	while((data->size = DMAXSIZE) && cptr->recvData(data.get())) {
		data->data[data->size] = 0;
		string header(string(data->data));
		size_t hlen = header.size();
		size_t dataOffset = hlen + 1;

		File::Size position;

		if(protocolVersion == 0)
		try {
			jsonObj_t h = jsonObj_t(&header);

			try {
				position = dynamic_cast<jsonInt_t &>(h.gie("position")).getVal();
			}
			catch(jsonNotExist &e) {
				if(dynamic_cast<jsonStr_t &>(h["action"]).getVal() == "finished") {
					return;
				} else throw;
			}
		}
		catch(exception &e) {
			sendErrMsg(cptr, e.what());
			fprintf(stderr, "Exception while receiving: %s\n", e.what());
			return;
		}

		if(protocolVersion == 1)
		try {
			if(header.size() > 0)
			try {
				jsonObj_t h(&header);
				if(dynamic_cast<jsonStr_t &>(h["action"]).getVal() == "finished") {
					return;
				}
			}
			catch(...) {
			}
			if(data->size < 8 + dataOffset) throw runtime_error("Message too short");

			// Conversion from MSB first position given in data to local variable
			position = 0;
			File::Size m = 1;
			for(size_t i = dataOffset + 7; i >= dataOffset; --i) {
				position += m*(unsigned char)data->data[i];
				m *= 256;
			}
			dataOffset += 8;
		} catch(exception &e) {
			sendErrMsg(cptr, e.what());
			LOG(Logger::Error, "Exception while receiving: %s", e.what());
			return;
		}

		//LOG(Logger::VerboseDebug, "Allocating new storage for data %zu - %zu", data->size, dataOffset);
		// TODO: improve it, so it won't need copying
		auto_ptr<anyData> rawData = allocData(data->size - dataOffset);
		memcpy(rawData->data, data->data + dataOffset, data->size - dataOffset);
		rawData->size = data->size - dataOffset;
		writer.writeData(position, rawData);

		/*fprintf(stderr, "Received %u bytes of data.\n", data->size);
		fflush(stderr);*/
	}
	D("Connection terminated");
}

void DataReceiver::run() {
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

void DataReceiver::sendHeader(jsonComponent_t &json) {
	string msg(json.toString());
	auto_ptr<anyData> msgdata = allocData(msg.size() + 1);
	cptr->sendData(msgdata.get());
}

bool DataReceiver::autoDelete() {
	return true;
}
