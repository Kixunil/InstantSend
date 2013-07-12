#include <stdexcept>
#include <cstdio>

#include "connectionreceiver.h"
#include "datareceiver.h"
#include "pluginlist.h"
#include "eventsink.h"
#include "sysapi.h"
#include "appcontrol.h"

using namespace InstantSend;
using namespace std;

int InstantSend::runningServers;
Mutex InstantSend::mRunningServers;

ConnectionReceiver::ConnectionReceiver(const string &plugin, const jsonComponent_t &config) : running(true), pluginName(plugin), pluginConfig(config.clone()), pref(PluginList::instance()[pluginName]), server(pref.as<ConnectionCreator>()->newServer(*pluginConfig)) {
	if(!server.valid()) throw runtime_error("Plugin instance creation failed");
}

bool ConnectionReceiver::checkRunning() {
	runningmutex.lock();
	bool tmp = running;
	runningmutex.unlock();
	return tmp;
}

void ConnectionReceiver::run() {
	pluginInstanceAutoPtr<Peer> client;

	mRunningServers.lock();
	++runningServers;
	mRunningServers.unlock();

	bcastServerStarted(*this);

	while((client = pluginInstanceAutoPtr<Peer>(server->acceptClient())).valid() && checkRunning()) {
		puts("Client connected.");
		(new DataReceiver(client))->start();
	}

	runningmutex.lock(); // synchronization trick
	runningmutex.unlock();

	mRunningServers.lock();
	--runningServers;
	mRunningServers.unlock();

	bcastServerStopped(*this);
	unfreezeMainThread();
}

void ConnectionReceiver::stop() throw() {
	runningmutex.lock();

	fprintf(stderr, "Stopping server.\n");

	running = false;

	try {
		dynamic_cast<AsyncStop &>(*server).stop();
		fprintf(stderr, "Server should stop immediately.\n");
	}
	catch(std::bad_cast &e) {
		// ignore error
		fprintf(stderr, "Sorry, you must wait for server to stop\n");
	}

	runningmutex.unlock();
}

const string &ConnectionReceiver::getPluginName() throw() {
	return pluginName;
}

const jsonComponent_t &ConnectionReceiver::getPluginConf() throw() {
	return *pluginConfig;
}

bool ConnectionReceiver::autoDelete() {
	fprintf(stderr, "ConnectionReceiver::autoDelete();\n");
	return true;
}

