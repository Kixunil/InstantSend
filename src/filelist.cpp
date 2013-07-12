#include <stdlib.h>
#include <cstdio>
#include <ctime>

#include "filelist.h"
#include "debug.h"

#ifdef WINDOWS
	#define random() rand()
#endif

using namespace InstantSend;
using namespace std;

fileController_t::fileController_t(int id) {
	identifier = id;
	refcount = 0;
}

void fileController_t::incRC() {
	refmutex.lock();
	++refcount;
	fprintf(stderr, "Controller RC: %d\n", refcount);
	refmutex.unlock();
}

void fileController_t::decRC() {
	refmutex.lock();
	fprintf(stderr, "Controller RC: %d\n", refcount - 1);
	if(!--refcount) {
		refmutex.unlock();
		zeroReferences();
	} else refmutex.unlock();
	++refSem;
}

int fileController_t::getId() {
	return identifier;
}

void fileController_t::waitZR() {
	while(refcount) --refSem;
}

fileController_t::~fileController_t() {
}

fileList_t &fileList_t::getList() {
	static fileList_t list;
	return list;
}

fileController_t &fileList_t::getController(int id, const string &fileName, File::Size fileSize, const string &machineID) {
	fileController_t *controller;
	D("getController");
	if(id) {
		listmutex.lock();
		map<int, fileController_t *>::iterator it = identifiers.find(id);
		if(it == identifiers.end()) {
			controller = insertController(id, fileName, fileSize, machineID);
		} else controller = it->second;
		listmutex.unlock();
	} else {
		D("Create new with random id");
		listmutex.lock();
		D("Got mutex");
		do {
			id = random();
			fprintf(stderr, "Trying id = %d\n", id);
			fflush(stderr);
		} while(identifiers.count(id));
		D("inserting controller");
		controller = insertController(id, fileName, fileSize, machineID);
		D("Releasing mutex");
		listmutex.unlock();
	}

	return *controller;
}

void fileList_t::removeController(int id, bool del) {
	MutexHolder mh(listmutex)
	D("Removing controller");
	map<int, fileController_t *>::iterator it;
	it = identifiers.find(id);
	if(it == identifiers.end()) {
		D("Not found");
		return;
	}
	D("Found");
	fileController_t *garbage = it->second;
	identifiers.erase(it);
	D("Controller removed");
	if(del) delete garbage;
	D("Deleted");
}
