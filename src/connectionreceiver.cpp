#include <stdexcept>
#include <cstdio>

#include "connectionreceiver.h"
#include "datareceiver.h"
#include "pluginlist.h"
#include "eventsink.h"
#include "sysapi.h"
#include "appcontrol.h"

int runningServers;
auto_ptr<mutex_t> mRunningServers = mutex_t::getNew();

connectionReceiver_t::connectionReceiver_t(const string &plugin, const jsonComponent_t &config) : runningmutex(mutex_t::getNew()), running(true), pluginName(plugin), pluginConfig(config.clone()), server(pluginList_t::instance()[pluginName].as<connectionCreator_t>()->newServer(*pluginConfig)) {}

bool connectionReceiver_t::checkRunning() {
	runningmutex->get();
	bool tmp = running;
	runningmutex->release();
	return tmp;
}

void connectionReceiver_t::run() {
	pluginInstanceAutoPtr<peer_t> client;

	mRunningServers->get();
	++runningServers;
	mRunningServers->release();

	bcastServerStarted(*this);

	while((client = pluginInstanceAutoPtr<peer_t>(server->acceptClient())).valid() && checkRunning()) {
		puts("Client connected.");
		(new dataReceiver_t(client))->start();
	}

	runningmutex->get(); // synchronization trick
	runningmutex->release();

	mRunningServers->get();
	--runningServers;
	mRunningServers->release();

	bcastServerStopped(*this);
	unfreezeMainThread();
}

void connectionReceiver_t::stop() throw() {
	runningmutex->get();

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

	runningmutex->release();
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

