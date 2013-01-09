#include <stdexcept>

#include "connectionreceiver.h"
#include "datareceiver.h"
#include "pluginlist.h"
#include "eventsink.h"
#include "sysapi.h"

connectionReceiver_t::connectionReceiver_t(const string &plugin, const jsonComponent_t &config) : runningmutex(mutex_t::getNew()), running(true), pluginName(plugin), pluginConfig(config.clone()), server(pluginList_t::instance()[pluginName].newServer(*pluginConfig)) {
	if(!server.get()) {
		const char *errmsg = pluginList_t::instance()[pluginName].lastError();
		if(errmsg)
			throw runtime_error("Could not load plugin " + pluginName + ": " + errmsg);
		else
			throw runtime_error("Could not load plugin " + pluginName);
	}

}

bool connectionReceiver_t::checkRunning() {
	runningmutex->get();
	bool tmp = running;
	runningmutex->release();
	return tmp;
}

void connectionReceiver_t::run() {
	try {
		auto_ptr<peer_t> client;
		bcastServerStarted(*this);

		while((client = auto_ptr<peer_t>(server->acceptClient())).get() && checkRunning()) {
			puts("Client connected.");
			(new dataReceiver_t(client))->start();
		}


	}
	catch(exception &e) {
		fprintf(stderr, "connectionReceiver failed: %s\n", e.what());
	}

	bcastServerStopped(*this);
}

void connectionReceiver_t::stop() {
	runningmutex->get();

	running = false;

	try {
		dynamic_cast<asyncStop_t &>(*server).stop();
	}
	catch(std::bad_cast &e) {
		// ignore error
	}

	runningmutex->release();
}

const string &connectionReceiver_t::getPluginName() {
	return pluginName;
}

const jsonComponent_t &connectionReceiver_t::getPluginConf() {
	return *pluginConfig;
}

bool connectionReceiver_t::autoDelete() {
	return true;
}

