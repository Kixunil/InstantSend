#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <memory>

#include "pluginapi.h"
#include "pluginlist.h"

int sendFile(auto_ptr<peer_t> client, FILE *file, const char *basename) {
	try {
		long fs;
		if(fseek(file, 0, SEEK_END) < 0) throw "Can't seek";
		if((fs = ftell(file)) < 0) throw "Unknown size";
		rewind(file);

		auto_ptr<anyData> data(allocData(DMAXSIZE+1));

		jsonObj_t msgobj;
		msgobj.insertNew("service", new jsonStr_t("filetransfer"));
		msgobj.insertNew("filename", new jsonStr_t(basename));
		msgobj.insertNew("filesize", new jsonInt_t(fs));
		string msg = msgobj.toString();
		strcpy(data->data, msg.c_str());
		data->size = msg.size() + 1;
		msgobj.deleteContent();

		if(!client->sendData(data.get())) throw "Can't send!";
		data->size = DMAXSIZE;
		if(!client->recvData(data.get())) throw "No response";
		data->data[data->size] = 0;

		msg = string(data->data);
		msgobj = jsonObj_t(&msg);
		if(dynamic_cast<jsonStr_t &>(msgobj.gie("service")) != "filetransfer") throw "Unknown protocol";
		jsonStr_t &action = dynamic_cast<jsonStr_t &>(msgobj.gie("action"));
		if(action.getVal() != string("accept")) {
			try {
				jsonStr_t &reason = dynamic_cast<jsonStr_t &>(msgobj.gie("reason"));
				throw (string("File rejected: ") + reason.getVal()).c_str();
			} catch(jsonNotExist) {
				throw "File rejected: unknown reason";
			}
		}

		auto_ptr<jsonInt_t> fp(new jsonInt_t(0));
		msgobj["position"] = fp.get();

		do {
			long fpos = ftell(file);
			if(fpos < 0) throw "Unknown position";
			fp->setVal(fpos);
			string msg = msgobj.toString();
			strcpy(data->data, msg.c_str());
			data->size = fread(data->data + msg.size() + 1, 1, 1022 - msg.size(), file);
			if(ferror(file)) throw "Can't read";
			data->size += msg.size() + 1;

			if(!client->sendData(data.get())) throw "Can't send!";
		} while(!feof(file));
		return 1;
	}
	catch(...) {
		return 0;
	}

}

auto_ptr<peer_t> findWay(jsonArr_t &ways) {
	pluginList_t &pl = pluginList_t::instance();
	for(int i = 0; i < ways.count(); ++i) {
		try {
			// Prepare plugin configuration
			jsonObj_t &way = dynamic_cast<jsonObj_t &>(*ways[i]);
			string pname = dynamic_cast<jsonStr_t &>(way.gie("plugin")).getVal();

			// Load plugin and try to connect
			auto_ptr<peer_t> client(pl[pname].newClient(&way.gie("config")));
			return client;
		}
		catch(...) {
			// continue trying
		}
	}
	return auto_ptr<peer_t>(NULL); // Return empty
}

int main(int argc, char **argv) {
	if(argc < 2) return 1;

	const char *home = getenv("HOME");
	if(!home) {
		fprintf(stderr, "HOME variable not specified");
		return 1;
	}

	string cfgfile = string(home) + string("/.instantsend/client.cfg");

	// Find out if config file was specified
	for(int i = 1; i+1 < argc; ++i) {
		if(string(argv[i]) == string("-c")) {
			cfgfile = string(argv[++i]);
			continue;
		}
	}

	try {
		pluginList_t &pl = pluginList_t::instance();
		pl.addSearchPath(string(home) + string("/.instantsend/plugins"));

		// Load configuration
		auto_ptr<jsonComponent_t> configptr(cfgReadFile(cfgfile.c_str()));
		jsonObj_t &config = dynamic_cast<jsonObj_t &>(*configptr.get());
		jsonObj_t &targets = dynamic_cast<jsonObj_t &>(config.gie("targets"));

		char *tname = NULL;
		for(int i = 1; i+1 < argc; ++i) {
			if(string(argv[i]) == string("-t")) {
				tname = argv[++i];
				continue;
			}

			if(string(argv[i]) == string("-f") && tname) {
				++i;
				// Open file
				FILE *file = fopen(argv[i], "r");
				if(!file) throw "Can't open file";

				// Find way to send file
				auto_ptr<peer_t> client = findWay(dynamic_cast<jsonArr_t &>(dynamic_cast<jsonObj_t &>(targets.gie(tname)).gie("ways")));
				if(!client.get()) {
					printf("Failed to connet to %s\n", tname);
					continue;
				}

				// Find base name TODO: make function and make it multi platform
				char *basename = argv[i], *ptr = argv[i];
				while(*ptr) if(*ptr++ == '/') basename = ptr;

				// Really send file
				if(sendFile(client, file, basename)) printf("File \"%s\" sent to \"%s\".\n", argv[i], tname);
			}
		}

		//string cfg = string("{ \"destIP\" : \"127.0.0.1\",  \"destPort\" : 4242 }");

		}
	catch(const char *msg) {
		char *dlerr = dlerror();
		fprintf(stderr, "Error: %s", msg);
		if(dlerr) fprintf(stderr, "; %s\n", dlerr); else putc('\n', stderr);
		return 1;
	}
	return 0;
}
