#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include <pthread.h>
#include <memory>
#include <stdexcept>

#include "pluginapi.h"
#include "pluginlist.h"
#include "sysapi.h"
#include "multithread.h"

string savedir;
string recvScript;

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

class clientThread_t : public thread_t {
	auto_ptr<peer_t> cptr;
	public:
		clientThread_t(auto_ptr<peer_t> peer) : cptr(peer) {
		}

		clientThread_t(peer_t *peer) : cptr(peer) {
		}

		void run() {	// This is called as new thread
			puts("Client thread started");
			peer_t &client = *cptr.get();
			auto_ptr<anyData> data = allocData(DMAXSIZE + 1);
			int received, hlen;
			string fname;
			FILE *file;

			data->size = DMAXSIZE;
			received = client.recvData(data.get());
			uint32_t fsize;
			if(received) {
				data->data[data->size] = 0; // make sure it won't owerflow
				string header = string(data->data); // extract header
				try {
					jsonObj_t h = jsonObj_t(&header);
					if(dynamic_cast<jsonStr_t &>(h.gie("service")) != "filetransfer") throw runtime_error("Service not supported");
					fname = dynamic_cast<jsonStr_t &>(h.gie("filename")).getVal();
					fsize = dynamic_cast<jsonInt_t &>(h.gie("filesize")).getVal();

					for(unsigned int i = 0; i < fname.size(); ++i) if(fname[i] == '/') fname[i] = '-'; //strip slashes TODO: use OS independent function
					printf("Recieving file %s (%d bytes)\n", fname.c_str(), fsize);
					file = fopen((savedir + fname).c_str(), "w");
					if(!file) throw "Can't open file";

					strcpy(data->data, "{ \"service\" : \"filetransfer\", \"action\" : \"accept\" }");
					data->size = 52;
					client.sendData(data.get());
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
					client.sendData(data.get());
					return;
				}
			} else return;

			int fileUncompleted = 1;

			do {
				string header;
				data->size = 1023;
				received = client.recvData(data.get());
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
						return;
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
			if(recvScript.size()) {
				pid_t pid = fork();
				if(!pid) {
					try {
						char *args[3];
						args[0] = new char[recvScript.size() + 1];
						strcpy(args[0], recvScript.c_str());
						args[1] = new char[fname.size() + savedir.size() + 1];
						strcpy(args[1], (savedir + fname).c_str());
						args[2] = NULL;
						execv(args[0], args);
					} catch(...) {
						// If it didn't execute, it will exit anyway
					}
					exit(1);
				}
			}

			return;
		}

		bool autoDelete() {
			return true;
		}

};

class serverThread_t : public thread_t {
	private:
		auto_ptr<serverPlugin_t> server;
	public:
		serverThread_t(serverPlugin_t *srv) : server(srv) {
		}

		serverThread_t(auto_ptr<serverPlugin_t> srv) : server(srv) {
		}

		void run() {
			peer_t *client;
			puts("Server thread started.");
			try {
				while((client = server->acceptClient())) {
					puts("Client connected.");
					clientThread_t *ct = new clientThread_t(client);
					if(!ct) puts("WTF?!"); else puts("Client thread created");
					ct->start();
				}

			}
			catch(exception &e) {
				fprintf(stderr, "Warning, plugin not loaded: %s\n", e.what());
			}
		}

		bool autoDelete() {
			return true;
		}
};

int main(int argc, char **argv) {
	pluginList_t &pl = pluginList_t::instance();

	// Prepare environment
	const char *home = getenv("HOME");
	if(!home) {
		fprintf(stderr, "HOME variable not specified");
		return 1;
	}
	try {
		string stddir = getStandardDir();
		pl.addSearchPath(combinePath(stddir, string("plugins")));
		savedir = combinePath(stddir, string("files"));
		string cfgfile = combinePath(stddir, string("server.cfg"));;

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

		try {
			recvScript = dynamic_cast<jsonStr_t &>(cfg.gie("onrecv")).getVal();
		}
		catch(...) {
			// Just ignore any error
		}

		try {
			savedir = dynamic_cast<jsonStr_t &>(cfg.gie("savedir")).getVal();
		}
		catch(...) {
		}

		try {
			jsonArr_t &pluginpaths = dynamic_cast<jsonArr_t &>(cfg.gie("pluginsearchpaths"));
			for(int i = 0; i < pluginpaths.count(); ++i) pl.addSearchPath(dynamic_cast<jsonStr_t &>(*pluginpaths[i]).getVal());
		}
		catch(...) {
		}

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
					(new serverThread_t(srv))->start();
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
