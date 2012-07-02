#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>
#include <memory>

#include "pluginapi.h"
#include "pluginlist.h"

string savedir;

class simpleException : public exception {
	private:
		string msg;
	public:
		inline simpleException(const char *str) {
			msg = string(str);
		}

		virtual const char* what() const throw() {
			return msg.c_str(); 
		}

		~simpleException() throw() {;}
};

void sendErrMsg(peer_t &client, const char *msg) {
	// Sends information about error to client
	jsonObj_t msgobj = jsonObj_t("{ \"action\" : \"error\" }"); // Little bit lazy method
	jsonStr_t tmp(msg);
	msgobj["message"] = &tmp;
	string retmsg = msgobj.toString();
	auto_ptr<anyData> data(allocData(retmsg.size() + 1));
	strcpy(data->data, retmsg.c_str());
	data->size = retmsg.size() + 1;
	client.sendData(data.get());
}

void *processClient(void *c) {	// This is called as new thread
	auto_ptr<peer_t> cptr((peer_t *)c);
	peer_t &client = *cptr.get();
	anyData *data = allocData(1024);
	int received, hlen;
	string fname;
	FILE *file;

	data->size = 1023;
	received = client.recvData(data);
	uint32_t fsize;
	if(received) {
		data->data[data->size] = 0; // make sure it won't owerflow
		string header = string(data->data); // extract header
		try {
			jsonObj_t h = jsonObj_t(&header);
			if(dynamic_cast<jsonStr_t &>(h.gie("service")) != "filetransfer") throw simpleException("Service not supported");
			fname = dynamic_cast<jsonStr_t &>(h.gie("filename")).getVal();
			fsize = dynamic_cast<jsonInt_t &>(h.gie("filesize")).getVal();

			for(unsigned int i = 0; i < fname.size(); ++i) if(fname[i] == '/') fname[i] = '-'; //strip slashes TODO: use OS independent function
			printf("Recieving file %s (%d bytes)\n", fname.c_str(), fsize);
			file = fopen((savedir + fname).c_str(), "w");
			if(!file) throw "Can't open file";

			strcpy(data->data, "{ \"service\" : \"filetransfer\", \"action\" : \"accept\" }");
			data->size = 52;
			client.sendData(data);
		}	
		catch(const exception &e) {
			if(file) fclose(file);
			// reply with information about error
			jsonObj_t msgobj = jsonObj_t("{ \"service\" : \"filetransfer\", \"action\" : \"reject\" }");
			jsonStr_t tmp;
			tmp.setVal(e.what());
			msgobj["reason"] = &tmp;
			string retmsg = msgobj.toString();
			strcpy(data->data, retmsg.c_str());
			data->size = retmsg.size() + 1;
			client.sendData(data);
			return NULL;
		}
	} else return NULL;

	int fileUncompleted = 1;

	do {
		string header;
		data->size = 1023;
		received = client.recvData(data);
		if(received) {
			data->data[data->size] = 0;
			header = string(data->data);
			hlen = strlen(data->data);
			try {
				jsonObj_t h = jsonObj_t(&header);

				uint32_t position = dynamic_cast<jsonInt_t &>(h.gie("position")).getVal();
				fseek(file, position, SEEK_SET);
				fwrite(data->data + hlen + 1, data->size - hlen - 1, 1, file);
				if(position + data->size - hlen - 1 == fsize) fileUncompleted = 0;
			}
			catch(const char *msg) {
				fclose(file);
				sendErrMsg(client, msg);
				return NULL;
			}
			catch(exception &e) {
				sendErrMsg(client, e.what());
			}

			/*fprintf(stderr, "Recieved %u bytes of data.\n", data->size);
			fflush(stderr);*/
//			write(1, data->data, data->size);
		} //else fprintf(stderr, "No data or error.\n");
	} while(received && fileUncompleted);
	fclose(file);

	fprintf(stderr, "Recieving finished.\n");

	return NULL;
}

void *startServer(void *config) {
	try {
		serverPlugin_t *server = (serverPlugin_t *)config;

		peer_t *client;
		while((client = server->acceptClient())) {
			pthread_t *clientthread = new pthread_t;
			pthread_create(clientthread, NULL, &processClient, client);
		}
	}
	catch(exception &e) {
		fprintf(stderr, "Warning, plugin not loaded: %s\n", e.what());
	}
	return NULL;
}

int main(int argc, char **argv) {
	pluginList_t &pl = pluginList_t::instance();

	// Prepare environment
	const char *home = getenv("HOME");
	if(!home) {
		fprintf(stderr, "HOME variable not specified");
		return 1;
	}
	try {
		// TODO: use platform independent function
		pl.addSearchPath(string(home) + string("/.instantsend/plugins"));
		savedir = string(home) + string("/.instantsend/files/");
		string cfgfile = string(home) + string("/.instantsend/server.cfg");

		// Process arguments
		for(int i = 1; i < argc; ++i) {
			if(string(argv[i]) == string("-c")) {
				if(i + 1 < argc) {
					cfgfile = string(argv[i + 1]);
				} else {
					fprintf(stderr, "Usage: -c CONFIG_FILE\n");
					return 1;
				}
			}
		}

		// Load configuration
		auto_ptr<jsonComponent_t> cfgptr = cfgReadFile(cfgfile.c_str());
		jsonObj_t &cfg = dynamic_cast<jsonObj_t &>(*cfgptr.get());

		jsonArr_t &complugins = dynamic_cast<jsonArr_t &>(cfg.gie("complugins"));

		// Load communication plugins
		for(int i = 0; i < complugins.count(); ++i) {
			try {
				// Get configuration
				jsonObj_t &plugin = dynamic_cast<jsonObj_t &>(*complugins[i]);
				jsonStr_t &pname = dynamic_cast<jsonStr_t &>(plugin.gie("name"));
				jsonComponent_t &pconf = plugin.gie("config");
				serverPlugin_t *srv;

				// Load plugin, create server instance and start new thread
				if((srv = pl[pname.getVal()].newServer(&pconf))) {
					pthread_t *thread = new pthread_t;
					pthread_create(thread, NULL, &startServer, srv);
				}
			} catch(exception &e) {
				fprintf(stderr, "Warning, plugin not loaded: %s\n", e.what());
			}
		}
	}
	catch(const char *msg) {
		char *dlerr = dlerror();
		if(dlerr) fprintf(stderr, "Error: %s; %s\n", msg, dlerror());
		else fprintf(stderr, "Error: %s", msg);

		return 1;
	}
	catch(exception &e) {
		fprintf(stderr, "Error: %s.\n", e.what());
		return 1;
	}
	catch(...) {
		fprintf(stderr, "Unknown error occured.\n"); // just in case, so program wont get stuck
		return 1;
	}

	pthread_exit(NULL);
}
