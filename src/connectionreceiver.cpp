#include <stdexcept>
#include <cstdio>

#include "connectionreceiver.h"
#include "datareceiver.h"
#include "pluginlist.h"
#include "eventsink.h"
#include "sysapi.h"
#include "appcontrol.h"

int runningServers;
Mutex mRunningServers;

connectionReceiver_t::connectionReceiver_t(const string &plugin, const jsonComponent_t &config) : running(true), pluginName(plugin), pluginConfig(config.clone()), pref(pluginList_t::instance()[pluginName]), server(pref.as<connectionCreator_t>()->newServer(*pluginConfig)) {
	if(!server.valid()) throw runtime_error("Plugin instance creation failed");
}

bool connectionReceiver_t::checkRunning() {
	runningmutex.lock();
	bool tmp = running;
	runningmutex.unlock();
	return tmp;
}

void connectionReceiver_t::run() {
	pluginInstanceAutoPtr<peer_t> client;

	mRunningServers.lock();
	++runningServers;
	mRunningServers.unlock();

	bcastServerStarted(*this);

	while((client = pluginInstanceAutoPtr<peer_t>(server->acceptClient())).valid() && checkRunning()) {
		puts("Client connected.");
		(new dataReceiver_t(client))->start();
	}

	runningmutex.lock(); // synchronization trick
	runningmutex.unlock();

	mRunningServers.lock();
	--runningServers;
	mRunningServers.unlock();

	bcastServerStopped(*this);
	unfreezeMainThread();
}

void connectionReceiver_t::stop() throw() {
	runningmutex.lock();

	fprintf(stderr, "Stopping server.\n");

	running = false;

	try {
		dynamic_cast<asyncStop_t &>(*server).stop();
		fprintf(stderr, "Server should stop immediately.\n");
	}
	catch(std::bad_cast &e) {
		// ignore error
		fprintf(stderr, "Sorry, you must wait for server to stop\n");
	}

	runningmutex.unlock();
}

const string &connectionReceiver_t::getPluginName() throw() {
	return pluginName;
}

const jsonComponent_t &connectionReceiver_t::getPluginConf() throw() {
	return *pluginConfig;
}

bool connectionReceiver_t::autoDelete() {
	fprintf(stderr, "connectionReceiver_t::autoDelete();\n");
	return true;
}

