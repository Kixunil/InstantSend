#include <stdlib.h>
#include <cstdio>
#include <ctime>

#include "filelist.h"
#include "debug.h"

#ifdef WINDOWS
	#define random() rand()
#endif

fileController_t::fileController_t(int id) {
	identifier = id;
	refcount = 0;
	refmutex = mutex_t::getNew();
}

void fileController_t::incRC() {
	refmutex->get();
	++refcount;
	refmutex->release();
}

void fileController_t::decRC() {
	refmutex->get();
	if(!--refcount) {
		refmutex->release();
		zeroReferences();
	} else refmutex->release();
}

int fileController_t::getId() {
	return identifier;
}

fileController_t::~fileController_t() {
}

fileList_t &fileList_t::getList() {
	static fileList_t list;
	return list;
}

fileController_t &fileList_t::getController(int id, const string &fileName, size_t fileSize, const string &machineID) {
	fileController_t *controller;
	D("getController")
	if(id) {
		listmutex->get();
		map<int, fileController_t *>::iterator it = identifiers.find(id);
		if(it == identifiers.end()) {
			controller = insertController(id, fileName, fileSize, machineID);
		} else controller = it->second;
		listmutex->release();
	} else {
		D("Create new with random id")
		listmutex->get();
		D("Got mutex")
		do {
			id = random();
			fprintf(stderr, "Trying id = %d\n", id);
			fflush(stderr);
		} while(identifiers.count(id));
		D("inserting controller")
		controller = insertController(id, fileName, fileSize, machineID);
		D("Releasing mutex")
		listmutex->release();
	}

	return *controller;
}

void fileList_t::removeController(int id, bool del) {
	listmutex->get();
	D("Removing controller")
	map<int, fileController_t *>::iterator it;
	it = identifiers.find(id);
	if(it == identifiers.end()) {
		D("Not found")
		listmutex->release();
		return;
	}
	D("Found")
	fileController_t *garbage = it->second;
	identifiers.erase(it);
	listmutex->release();
	D("Controller removed")
	if(del) delete garbage;
	D("Deleted")
}
