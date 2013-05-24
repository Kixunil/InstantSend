#include <stdexcept>
#include <string.h>
#include <cstdio>

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

bool dataFragment_t::writeData(WritableFile &file) const {
	file.write(dat->data, dat->size, pos);
	//fprintf(stderr, "Written %zu bytes of data\n", dat->size);
	return true;
}

fileWriter_t::fileWriter_t(int id, const string &fileName, size_t fileSize, const string &machineId) : fileController_t(id), inputSem(MAX_BUF_FRAGMENT_COUNT), file(fileName.c_str()) {
	fName = fileName;
	fSize = fileSize;
	mId = machineId;
	lastpos = 0;
	lastBatch = 0;
	bytes = 0;
	stop = false;
	hasStopped = false;
	hardPause = false;
	zr = true;
	lastUpdate = 0;
	updateInterval = 5000;
	tStatus = IS_TRANSFER_IN_PROGRESS;
	start();
	bcastProgressBegin(*this);
	D("broadcasted onBegin event");
}

string fileWriter_t::getFileName() {
	DMG;
	mutex.lock();        
	string fn = fName;
	mutex.unlock();    
	return fn;           
}

File::Size fileWriter_t::getFileSize() {
	DMG;
	mutex.lock();
	size_t fs = fSize;
	mutex.unlock();
	return fs;
}

string fileWriter_t::getMachineId() {
	DMG;
	mutex.lock();
	string mi = mId;
	mutex.unlock();
	return mi;
}

int fileWriter_t::getTransferStatus() {
	DMG;
	mutex.lock();
	int ts = tStatus;
	mutex.unlock();
	return ts;
}

char fileWriter_t::getDirection() {
	return IS_DIRECTION_DOWNLOAD;
}

bool fileWriter_t::empty() {
	DMG;
	mutex.lock();
	bool e = queue.empty();
	mutex.unlock();
	return e;
}

void fileWriter_t::writeBuffer() {
	--outputSem;
	if(stop) return;

	mutex.lock();
	dataFragment_t fragment(queue.top());
	queue.pop();
	mutex.unlock();

	++inputSem;
	while(!fragment.writeData(file) && !stop) {
		pause();
		pausePoint();
	}
	if(stop) return;
	bytes += fragment.size();
	fragment.freeData();
	if(!(lastUpdate = (lastUpdate + 1) % updateInterval)) bcastProgressUpdate(*this);
}

void fileWriter_t::run() {
	zr = false;
	while(bytes < fSize && !stop) {
		writeBuffer();
	}

	// try write remaining data (aborts on failure)
	mutex.lock();
	while(queue.size() && queue.top().writeData(file)) {
		const dataFragment_t &fragment(queue.top());

		bytes += fragment.size();
		fragment.freeData();
		queue.pop();
	}
	mutex.unlock();

#ifdef DEBUG
	fprintf(stderr, "Transferred %zu bytes out of %zu\n", bytes, fSize);
	fflush(stderr);
#endif

	if(tStatus == IS_TRANSFER_IN_PROGRESS) {
		if(bytes == fSize) tStatus = IS_TRANSFER_FINISHED;
		else tStatus = IS_TRANSFER_ERROR;
	}

	stop = true;
	bcastProgressUpdate(*this);
	bcastProgressEnd(*this);
	waitZR();
	cleanupCheck(true);
	hasStopped = true;
}

bool fileWriter_t::writeData(long position, auto_ptr<anyData> &data) {
	DMG;
	// Just in case, so we don't insert empty data
	if(!data->size) {
		data.reset();
		return true;
	}

	--inputSem;

	mutex.lock();
	if(lastpos > position) ++lastBatch;
	queue.push(dataFragment_t(data, position, lastBatch));
	mutex.unlock();

	++outputSem;

	return true;
}

bool fileWriter_t::autoDelete() {
	return zr && hasStopped;
}

File::Size fileWriter_t::getTransferredBytes() {
	return bytes;
}

void fileWriter_t::pauseTransfer() {
	DMG;
	mutex.lock();
	hardPause = true;
	pause();
	mutex.unlock();
}

void fileWriter_t::resumeTransfer() {
	DMG;
	mutex.lock();
	hardPause = false;
	resume();
	mutex.unlock();
}

bool fileWriter_t::cleanupCheck(bool calledByThisThread) {
	DMG;
	mutex.lock();
	if(zr && hasStopped) {
		mutex.unlock();
		fileList_t::getList().removeController(getId(), !calledByThisThread);
		return true;
	}
	mutex.unlock();
	return false;
}

void fileWriter_t::zeroReferences() {
	DMG;
	mutex.lock();
	zr = true;
	mutex.unlock();
	if(!cleanupCheck()) {
		DMG;
		mutex.lock();
		stop = true;
		resume();
		++outputSem;
		mutex.unlock();
	}
}

fileWriter_t::~fileWriter_t() {
	D("Destroying writer")
	mutex.lock();
	D("Got mutex")
	while(!queue.empty()) {
		queue.top().freeData();
		queue.pop();
	}
	D("Data deleted")
	mutex.unlock();
	D("Mutex released")
}

fileStatus_t::~fileStatus_t() {
}

fileController_t *fileList_t::insertController(int id, const string &fileName, File::Size fileSize, const string &machineID) {
	jsonInt_t tmp((intL_t)fileSize);
	fprintf(stderr, "Creating controller for file %s (%sB) with id %d\n", fileName.c_str(), tmp.toString().c_str(), id);
	fflush(stderr);
	fileController_t *writer = new fileWriter_t(id, fileName, fileSize, machineID);
	identifiers.insert(pair<int, fileController_t *>(id, writer));
	return writer;
}
