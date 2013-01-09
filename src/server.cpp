#ifdef WINDOWS
	#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WINDOWS
	#include <dlfcn.h>
	#include <pthread.h>
#endif
#include <unistd.h>
#include <memory>

#include "pluginapi.h"
#include "pluginlist.h"
#include "appcontrol.h"
#include "sysapi.h"
#include "eventsink.h"
#include "connectionreceiver.h"

string savedir;
//string recvScript;

int main(int argc, char **argv) {
	onAppStart(argc, argv);
	pluginList_t &pl = pluginList_t::instance();

	try {
		string userdir = getUserDir();
		savedir = combinePath(userdir, string("files"));
		string cfgfile = combinePath(userdir, string("server.cfg"));;

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
			savedir = dynamic_cast<jsonStr_t &>(cfg.gie("savedir")).getVal();
		}
		catch(...) {
		}

// Explicitly defined paths have bigger priority than system path, which has bigger priority than user path
		try {
			jsonArr_t &pluginpaths = dynamic_cast<jsonArr_t &>(cfg.gie("pluginsearchpaths"));
			for(int i = 0; i < pluginpaths.count(); ++i) pl.addSearchPath(dynamic_cast<jsonStr_t &>(*pluginpaths[i]).getVal());
		}
		catch(...) {
		}
		pl.addSearchPath(getSystemPluginDir()); 
		pl.addSearchPath(combinePath(userdir, string("plugins")));

		try {
			eventSink_t::instance().autoLoad(dynamic_cast<jsonObj_t &>(cfg.gie("eventhandlers")));
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

				(new connectionReceiver_t(pname.getVal(), pconf))->start();
				// Load plugin, create server instance and start new thread
			} catch(exception &e) {
				fprintf(stderr, "Warning, plugin not loaded: %s\n", e.what());
			}
		}
	}
	catch(const char *msg) {
#ifndef WINDOWS
		char *dlerr = dlerror();
		if(dlerr) fprintf(stderr, "Error: %s; %s\n", msg, dlerror());
		else fprintf(stderr, "Error: %s", msg);
#else
		fprintf(stderr, "Error: %s", msg);
#endif

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

	while(!stopApp) {
		freezeMainThread();
	}

	onAppStop();
}
