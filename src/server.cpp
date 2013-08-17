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
#include "sysapi.h"
#include "eventsink.h"
#include "serverlist.h"
#include "internalevents.h"

using namespace std;

namespace InstantSend {
	string savedir;
}

using namespace InstantSend;
//string recvScript;

extern unsigned int threadCount;

bool checkRunningServers() {
	mRunningServers.lock();
	bool result = runningServers;
	mRunningServers.unlock();
	return result;
}

int main(int argc, char **argv) {
	onAppStart(argc, argv);
	auto_ptr<Application> app(new Application(argc, argv));
	instantSend = app.get();
	InternalEventHandler ieh; // no need to do anything - it registers and unregisters itself automatically
	PluginList &pl = PluginList::instance();
	pl.setSink(EventSink::instance());
	auto_ptr<jsonComponent_t> cfgptr;

	try {
		string userdir = getUserDir();
		savedir = combinePath(userdir, "files");
		string cfgfile = combinePath(userdir, "server.cfg");

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
		LOG(Logger::Debug, "Loading configuration");
		cfgptr = cfgReadFile(cfgfile.c_str());
		jsonObj_t &cfg = dynamic_cast<jsonObj_t &>(*cfgptr.get());

		try {
			savedir = dynamic_cast<jsonStr_t &>(cfg.gie("savedir")).getVal();
		}
		catch(...) {
		}

// Explicitly defined paths have bigger priority than system path, which has bigger priority than user path
		LOG(Logger::Debug, "Loading plugin search paths");
		try {
			jsonArr_t &pluginpaths = dynamic_cast<jsonArr_t &>(cfg.gie("pluginsearchpaths"));
			for(int i = 0; i < pluginpaths.count(); ++i) pl.addSearchPath(dynamic_cast<jsonStr_t &>(pluginpaths[i]).getVal());
		}
		catch(...) {
		}
		pl.addSearchPath(getSystemPluginDir()); 
		pl.addSearchPath(combinePath(userdir, string("plugins")));

		LOG(Logger::Debug, "Loading events");
		try {
			EventSink::instance().autoLoad(dynamic_cast<jsonObj_t &>(cfg.gie("eventhandlers")));
		}
		catch(exception &e) {
			LOG(Logger::Note, "No event plugins loaded: %s", e.what());
		}

		try {
			BPluginRef secstorplug(PluginList::instance()[dynamic_cast<jsonStr_t &>(cfg["securestorage_plugin"]).getVal()]);
			jsonComponent_t *sscfg;
			try {
				sscfg = &cfg["securestorage_config"];
			}
			catch(...) {
				sscfg = NULL;
			}
			instantSend->secureStorage(secstorplug.as<SecureStorageCreator>()->openSecureStorage(sscfg));
			/* TEST: if(instantSend->secureStorage()->setSecret("Hello", "Hello world!")) LOG(Logger::Note, "Secret stored.");
			else LOG(Logger::Note, "Storing unsuccessful."); */
		}
		catch(exception &e) {
			LOG(Logger::Note, "Secure storage plugin not loaded: %s", e.what());
		}

		LOG(Logger::Debug, "Loading servers");
		jsonComponent_t &comp(cfg.gie("complugins"));
		jsonArr_t &complugins = dynamic_cast<jsonArr_t &>(comp);

		// Load communication plugins
		for(int i = 0; i < complugins.count(); ++i) {
			try {
				// Get configuration
				jsonObj_t &plugin = dynamic_cast<jsonObj_t &>(complugins[i]);
				jsonStr_t &pname = dynamic_cast<jsonStr_t &>(plugin.gie("name"));
				jsonComponent_t &pconf = plugin.gie("config");

				serverList_t::instance().add(pname.getVal(), pconf);
				// Load plugin, create server instance and start new thread
			} catch(exception &e) {
				LOG(Logger::Warning, "Plugin not loaded: %s", e.what());
			}
		}
	}
	catch(exception &e) {
		LOG(Logger::Error, "Error: %s.", e.what());
		return 1;
	}
	catch(...) {
		LOG(Logger::Error, "Unknown error occured."); // just in case, so program wont get stuck
		return 1;
	}

	while(!stopApp && pl.count() && threadCount) {
		freezeMainThread();
	}

	serverList_t::instance().remove();
	EventSink::instance().unloadAllEventPlugins();

	while(!stopAppFast && pl.count() && threadCount) {
		freezeMainThread();
		//printf("Plugin count: %u\n", pl.count());
	}

	onAppStop();
}
