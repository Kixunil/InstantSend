#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <memory>

#include "pluginapi.h"
#include "pluginlist.h"

int main(int argc, char **argv) {
	if(argc < 2) return 1;

	pluginList_t &pl = *pluginList_t::instance();
	const char *home = getenv("HOME");
	if(!home) {
		fprintf(stderr, "HOME variable not specified");
		return 1;
	}
	try {
		pl.addSearchPath(string(home) + string("/.instantsend/plugins"));

		string cfg = string("{ \"destIP\" : \"127.0.0.1\",  \"destPort\" : 4242 }");

		jsonObj_t *cconfig = new jsonObj_t(&cfg);

		clientPlugin_t *client = pl["ip4tcp"].newClient(NULL);

		if(!client->connectTarget(cconfig)) {
			fprintf(stderr, "Can't connect!\n");
			return 2;
		}

		anyData *data = allocData(1024);
//		char buf[512];
		char *basename = argv[1], *ptr = argv[1];
		long fs;
		while(*ptr) if(*ptr++ == '/') basename = ptr;
		
		FILE *file = fopen(argv[1], "r");
		if(!file) throw "Can't open file";
		if(fseek(file, 0, SEEK_END) < 0) throw "Can't seek";
		if((fs = ftell(file)) < 0) throw "Unknown size";
		rewind(file);

		jsonObj_t msgobj;
		msgobj.insertNew("service", new jsonStr_t("filetransfer"));
		msgobj.insertNew("filename", new jsonStr_t(basename));
		msgobj.insertNew("filesize", new jsonInt_t(fs));
		string msg = msgobj.toString();
		strcpy(data->data, msg.c_str());
		data->size = msg.size() + 1;
		msgobj.deleteContent();

		if(!client->sendData(data)) throw "Can't send!";
		data->size = 1023;
		if(!client->recvData(data)) throw "No response";
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

			if(!client->sendData(data)) throw "Can't send!";
		} while(!feof(file));
	}
	catch(const char *msg) {
		char *dlerr = dlerror();
		fprintf(stderr, "Error: %s", msg);
		if(dlerr) fprintf(stderr, "; %s\n", dlerr); else putc('\n', stderr);
		return 1;
	}
	return 0;
}
