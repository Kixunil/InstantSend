#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <memory>
#include <stdexcept>
#include <string.h>

#include "multithread.h"
#include "appcontrol.h"
#include "windows-appcontrol.h"

class WindowsMutexData : public Mutex::Data {
	private:
		CRITICAL_SECTION critical;
	public:
		WindowsMutexData() {
			InitializeCriticalSection(&critical);
		}

		void lock() {
			EnterCriticalSection(&critical);
		}

		void unlock() {
			LeaveCriticalSection(&critical);
		}

		~WindowsMutexData() {
			DeleteCriticalSection(&critical);
		}
};

Mutex::Mutex() : mData(new WindowsMutexData()) {}
Mutex::Data::~Data() {}

class windowsThreadData_t : public threadData_t {
	private:
		const windowsThreadData_t &operator=(const windowsThreadData_t &data) {
			throw runtime_error("Unsupported operation"); // make sure it won't be used
		}
	public:
		HANDLE threadHandle;
		unsigned threadID;
		Mutex mutex, pausemutex, pausectrlmutex;
/*		CONDITION_VARIABLE condition;
		CRITICAL_SECTION condmutex;*/
		thread_t *parent;
		bool running;
		bool paused;
		windowsThreadData_t() {
		}

		~windowsThreadData_t() {
		}
};

class WindowsSemaphoreData : public Semaphore::Data {
	public:
		WindowsSemaphoreData(unsigned int initVal = 0) : semaphore(CreateSemaphore(NULL, initVal, 2147483647, 0)) {}

		void operator++() {
			ReleaseSemaphore(semaphore, 1, 0);
		}

		void operator--() {
			WaitForSingleObject(semaphore, INFINITE);
		}

		~WindowsSemaphoreData() {
			CloseHandle(semaphore);
		}

	private:
		HANDLE semaphore;
};

Semaphore::Semaphore(unsigned int initVal) : mSemData(new WindowsSemaphoreData(initVal))  {}
Semaphore::Data::~Data() {}

void delThread(void *thread) {
	windowsThreadData_t *t = (windowsThreadData_t *)thread;
	t->mutex.lock();
	t->running = false;
	t->mutex.unlock();
	if(t->parent->autoDelete()) delete t->parent;
}

unsigned WINAPI startThread(LPVOID thread) {
	windowsThreadData_t *td = (windowsThreadData_t *)thread;
	td->running = true;
	td->paused = false;
	td->parent->run();
	return 0;
}

void thread_t::start() {

	threadData = auto_ptr<threadData_t>(new windowsThreadData_t());
	windowsThreadData_t &tdata = dynamic_cast<windowsThreadData_t &>(*threadData.get());
	tdata.mutex.lock();
	tdata.parent = this;
	EnterCriticalSection(&threadCountMutex);
	tdata.threadHandle = (HANDLE)_beginthreadex(NULL, 0, startThread, (void *)&tdata, 0, &tdata.threadID);
	if(!tdata.threadHandle) {
		LeaveCriticalSection(&threadCountMutex);
		throw runtime_error("Couldn't start thread");
	}
	++threadCount;
	LeaveCriticalSection(&threadCountMutex);
	//if(autoDelete()) pthread_detach(tdata.thread);
	tdata.mutex.unlock();
}

void thread_t::pause() {
	windowsThreadData_t &tdata = dynamic_cast<windowsThreadData_t &>(*threadData.get());
	SuspendThread(tdata.threadHandle);
	//EnterCriticalSection(&tdata.condmutex);
	/*tdata.pausectrlmutex.lock();
	tdata.paused = true;
	tdata.pausectrlmutex.unlock();
	*/
	//LeaveCriticalSection(&tdata.condmutex);
}

void thread_t::resume() {
	windowsThreadData_t &tdata = dynamic_cast<windowsThreadData_t &>(*(threadData.get()));
	ResumeThread(tdata.threadHandle);
//	EnterCriticalSection(&tdata.condmutex);
	/*tdata.pausectrlmutex.lock();
	if(tdata.paused) {
		tdata.paused = false;
		tdata.pausemutex.unlock();
	}
	tdata.pausectrlmutex.unlock();*/
/*	LeaveCriticalSection(&tdata.condmutex);
	WakeConditionVariable(&tdata.condition);*/
}

bool thread_t::running() {
	windowsThreadData_t &tdata = dynamic_cast<windowsThreadData_t &>(*threadData.get());
	tdata.pausectrlmutex.lock();
	bool paused = tdata.paused;
	tdata.pausectrlmutex.unlock();
	return !paused;
}

void thread_t::pausePoint() {
	windowsThreadData_t &tdata = dynamic_cast<windowsThreadData_t &>(*threadData.get());
	//EnterCriticalSection(&tdata.condmutex);
	tdata.pausectrlmutex.lock();
	while(tdata.paused) {
		//SleepConditionVariableCS(&tdata.condition, &tdata.condmutex, INFINITE);
		tdata.pausectrlmutex.unlock();
		tdata.pausemutex.lock(); // This will cause intentional deadlock until thread is resumed
		tdata.pausectrlmutex.lock();
	}
	//LeaveCriticalSection(&tdata.condmutex);
}

threadData_t::~threadData_t() {;}

thread_t::~thread_t() {;}
