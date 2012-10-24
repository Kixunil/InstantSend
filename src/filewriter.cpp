#include <stdexcept>
#include <string.h>

#include "filewriter.h"
#include "debug.h"
#include "eventsink.h"

#define DMG // fprintf(stderr, "Getting mutex at %d\n", __LINE__); fflush(stderr);

dataFragment_t::dataFragment_t(auto_ptr<anyData> &data, long position, int batchNumber) {
	dat = data.release();
	pos = position;
	batch = batchNumber;
}

bool dataFragment_t::operator<(const dataFragment_t &df) const {
	return batch < df.batch && pos < df.pos;
}

bool dataFragment_t::writeData(FILE *file) const {
	flockfile(file);
	fseek(file, pos, SEEK_SET);
	bool success = fwrite(dat->data, dat->size, 1, file);
	if(!success) {
		printf("Error occured: %s\n", strerror(ferror(file)));
		fflush(stdout);
	}
	funlockfile(file);
	return success;
}

fileWriter_t::fileWriter_t(int id, const string &fileName, size_t fileSize, const string &machineId) : fileController_t(id) {
	mutex = mutex_t::getNew();
	fName = fileName;
	fSize = fileSize;
	mId = machineId;
	lastpos = 0;
	lastBatch = 0;
	bytes = 0;
	bufsiz = 0;
	stop = false;
	hardPause = false;
	zr = false;
	lastUpdate = 0;
	updateInterval = 500;
	tStatus = IS_TRANSFER_IN_PROGRESS;
	f = fopen(fName.c_str(), "w");
	if(!f) throw runtime_error("Can't open file \"" + fName + " for writting.");
	start();
	bcastProgressBegin(*this);
	D("broadcasted onBegin event");
}

string fileWriter_t::getFileName() {
	DMG;
	mutex->get();        
	string fn = fName;
	mutex->release();    
	return fn;           
}

size_t fileWriter_t::getFileSize() {
	DMG;
	mutex->get();
	size_t fs = fSize;
	mutex->release();
	return fs;
}

string fileWriter_t::getMachineId() {
	DMG;
	mutex->get();
	string mi = mId;
	mutex->release();
	return mi;
}

int fileWriter_t::getTransferStatus() {
	DMG;
	mutex->get();
	int ts = tStatus;
	mutex->release();
	return ts;
}

char fileWriter_t::getDirection() {
	return IS_DIRECTION_DOWNLOAD;
}

bool fileWriter_t::empty() {
	DMG;
	mutex->get();
	bool e = queue.empty();
	mutex->release();
	return e;
}

void fileWriter_t::writeBuffer() {
	while(!empty()) {
		DMG;
		mutex->get();
		if(!queue.top().writeData(f)) {
			mutex->release();
			pause();
			pausePoint();
		} else {
			bytes += queue.top().size();
			bufsiz -= queue.top().size();
			queue.top().freeData();
			queue.pop();
			for(set<thread_t *>::iterator it = waitingThreads.begin(); it != waitingThreads.end(); ++it) {
				(*it)->resume();
				waitingThreads.erase(it);
			}
			mutex->release();
			if(!(lastUpdate = (lastUpdate + 1) % updateInterval)) bcastProgressUpdated(*this);
		}
	}
}

bool fileWriter_t::shouldRun() {
	mutex->get();
	bool result = bytes < fSize && !stop;
	mutex->release();
	return result;
}

void fileWriter_t::run() {
	while(shouldRun()) {
		writeBuffer();

		if(empty()) pause();
		pausePoint();
	}

	writeBuffer(); // try write remaining data

#ifdef DEBUG
	fprintf(stderr, "Transferred %zu bytes out of %zu\n", bytes, fSize);
	fflush(stderr);
#endif

	mutex->get();
	if(tStatus == IS_TRANSFER_IN_PROGRESS) {
		if(bytes == fSize) tStatus = IS_TRANSFER_FINISHED;
		else tStatus = IS_TRANSFER_ERROR;
	}

	stop = true;
	mutex->release();
	bcastProgressUpdated(*this);
	cleanupCheck(true);
	
}

bool fileWriter_t::writeData(long position, auto_ptr<anyData> &data, thread_t &caller) {
	DMG;
	mutex->get();
	if(hardPause || data->size + bufsiz > MAXBUFSIZE) {
		caller.pause();
		waitingThreads.insert(&caller);
		mutex->release();
		return false;
	}

	if(lastpos > position) ++lastBatch;
	bufsiz += data->size;
	queue.push(dataFragment_t(data, position, lastBatch));
	if(!running()) resume();
	mutex->release();
	return true;
}

bool fileWriter_t::autoDelete() {
	return zr;
}

size_t fileWriter_t::getTransferredBytes() {
	return bytes;
}

void fileWriter_t::pauseTransfer() {
	DMG;
	mutex->get();
	hardPause = true;
	pause();
	mutex->release();
}

void fileWriter_t::resumeTransfer() {
	DMG;
	mutex->get();
	hardPause = false;
	resume();
	mutex->release();
}

bool fileWriter_t::cleanupCheck(bool calledByThisThread) {
	DMG;
	mutex->get();
	if(zr && stop) {
		mutex->release();
		bcastProgressEnded(*this);
		fileList_t::getList().removeController(getId(), !calledByThisThread);
		return true;
	}
	mutex->release();
	return false;
}

void fileWriter_t::zeroReferences() {
	DMG;
	mutex->get();
	zr = true;
	mutex->release();
	if(!cleanupCheck()) {
		DMG;
		mutex->get();
		stop = true;
		resume();
		mutex->release();
	}
}

fileWriter_t::~fileWriter_t() {
	D("Destroying writer")
	mutex->get();
	D("Got mutex")
	if(f) fclose(f);
	D("File closed")
	while(!queue.empty()) {
		queue.top().freeData();
		queue.pop();
	}
	D("Data deleted")
	mutex->release();
	D("Mutex released")
}

fileStatus_t::~fileStatus_t() {
}

fileController_t *fileList_t::insertController(int id, const string &fileName, size_t fileSize, const string &machineID) {
	fprintf(stderr, "Creating controller for file %s (%zuB) with id %d\n", fileName.c_str(), fileSize, id);
	fflush(stderr);
	fileController_t *writer = new fileWriter_t(id, fileName, fileSize, machineID);
	identifiers.insert(pair<int, fileController_t *>(id, writer));
	return writer;
}
