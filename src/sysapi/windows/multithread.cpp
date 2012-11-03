#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <memory>
#include <stdexcept>
#include <string.h>

#include "multithread.h"

class windowsCriticalSection_t : public mutex_t {
	private:
		CRITICAL_SECTION critical;
	public:
		windowsCriticalSection_t() {
			InitializeCriticalSection(&critical);
		}

		void get() {
			EnterCriticalSection(&critical);
		}

		void release() {
			LeaveCriticalSection(&critical);
		}

		~windowsCriticalSection_t() {
			DeleteCriticalSection(&critical);
		}
};

auto_ptr<mutex_t> mutex_t::getNew() {
	return auto_ptr<mutex_t>(new windowsCriticalSection_t());
}

class windowsThreadData_t : public threadData_t {
	private:
		const windowsThreadData_t &operator=(const windowsThreadData_t &data) {
			throw runtime_error("Unsupported operation"); // make sure it won't be used
		}
	public:
		HANDLE threadHandle;
		unsigned threadID;
		auto_ptr<mutex_t> mutex, pausemutex, pausectrlmutex;
/*		CONDITION_VARIABLE condition;
		CRITICAL_SECTION condmutex;*/
		thread_t *parent;
		bool running;
		bool paused;
		windowsThreadData_t() {
			mutex = mutex_t::getNew();
			/*InitializeCriticalSection(condmutex);
			InitializeConditionVariable(condition);*/
			pausemutex = mutex_t::getNew();
			pausemutex->get();
			pausectrlmutex = mutex_t::getNew();
		}

		~windowsThreadData_t() {
			pausectrlmutex->get();
			/*if(paused) {
				pausemutex->release();
			}*/
			pausemutex->release();
			pausectrlmutex->release();
			mutex->get();

			//if(running) pthread_exit(&thread);
			/*DeleteCriticalSection(&condmutex);
			DeleteConditionVariable(&condition);*/
			mutex->release();
		}
};

void delThread(void *thread) {
	windowsThreadData_t *t = (windowsThreadData_t *)thread;
	t->mutex->get();
	t->running = false;
	t->mutex->release();
	if(t->parent->autoDelete()) delete t->parent;
}

unsigned WINAPI startThread(LPVOID thread) {
	windowsThreadData_t *td = (windowsThreadData_t *)thread;
	td->running = true;
	td->paused = false;
	//pthread_cleanup_push(&delThread, thread); // TODO: find windows equivalent
	td->parent->run();
	//pthread_cleanup_pop(1);
	return 0;
}

void thread_t::start() {

	threadData = auto_ptr<threadData_t>(new windowsThreadData_t());
	windowsThreadData_t &tdata = dynamic_cast<windowsThreadData_t &>(*threadData.get());
	tdata.mutex->get();
	tdata.parent = this;
	tdata.threadHandle = (HANDLE)_beginthreadex(NULL, 0, startThread, (void *)&tdata, 0, &tdata.threadID);
	if(!tdata.threadHandle) throw runtime_error("Couldn't start thread");
	//if(autoDelete()) pthread_detach(tdata.thread);
	tdata.mutex->release();
}

void thread_t::pause() {
	windowsThreadData_t &tdata = dynamic_cast<windowsThreadData_t &>(*threadData.get());
	//EnterCriticalSection(&tdata.condmutex);
	tdata.pausectrlmutex->get();
	tdata.paused = true;
	tdata.pausectrlmutex->release();
	//LeaveCriticalSection(&tdata.condmutex);
}

void thread_t::resume() {
	windowsThreadData_t &tdata = dynamic_cast<windowsThreadData_t &>(*(threadData.get()));
//	EnterCriticalSection(&tdata.condmutex);
	tdata.pausectrlmutex->get();
	if(tdata.paused) {
		tdata.paused = false;
		tdata.pausemutex->release();
	}
	tdata.pausectrlmutex->release();
/*	LeaveCriticalSection(&tdata.condmutex);
	WakeConditionVariable(&tdata.condition);*/
}

bool thread_t::running() {
	windowsThreadData_t &tdata = dynamic_cast<windowsThreadData_t &>(*threadData.get());
	tdata.pausectrlmutex->get();
	bool paused = tdata.paused;
	tdata.pausectrlmutex->release();
	return !paused;
}

void thread_t::pausePoint() {
	windowsThreadData_t &tdata = dynamic_cast<windowsThreadData_t &>(*threadData.get());
	//EnterCriticalSection(&tdata.condmutex);
	tdata.pausectrlmutex->get();
	while(tdata.paused) {
		//SleepConditionVariableCS(&tdata.condition, &tdata.condmutex, INFINITE);
		tdata.pausectrlmutex->release();
		tdata.pausemutex->get(); // This will cause intentional deadlock until thread is resumed
		tdata.pausectrlmutex->get();
	}
	//LeaveCriticalSection(&tdata.condmutex);
}

mutex_t::~mutex_t() {;}

threadData_t::~threadData_t() {;}

thread_t::~thread_t() {;}
