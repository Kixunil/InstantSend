#include <stdio.h>
#include <pthread.h>
#include <memory>
#include <stdexcept>
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <errno.h>

#include "multithread.h"
#include "posix-appcontrol.h"

class linuxMutex_t : public mutex_t {
	private:
		pthread_mutex_t mutex;
	public:
		linuxMutex_t() {
			if(pthread_mutex_init(&mutex, NULL)) throw runtime_error("Can't create mutex");
		}

		void get() {
			pthread_mutex_lock(&mutex);
		}

		void release() {
			pthread_mutex_unlock(&mutex);
		}

		~linuxMutex_t() {
			pthread_mutex_destroy(&mutex);
		}
};

auto_ptr<mutex_t> mutex_t::getNew() {
	return auto_ptr<mutex_t>(new linuxMutex_t());
}

class linuxThreadData_t : public threadData_t {
	private:
		const linuxThreadData_t &operator=(const linuxThreadData_t &data) {
			throw runtime_error("Unsupported operation"); // make sure it won't be used
		}
	public:
		pthread_t thread;
		pthread_cond_t condition;
		pthread_mutex_t condmutex;
		thread_t *parent;
		//pthread_attr_t threadAttr;
		auto_ptr<mutex_t> mutex;
		bool running;
		bool paused;
		linuxThreadData_t() {
			mutex = mutex_t::getNew();
			int errn;
			if((errn = pthread_mutex_init(&condmutex, NULL))) throw runtime_error(string("Failed to initialize mutex: ") + strerror(errn));
			pthread_cond_init(&condition, NULL);
		}

		~linuxThreadData_t() {
			mutex->get();
			if(running) pthread_exit(&thread);
			pthread_mutex_destroy(&condmutex);
			pthread_cond_destroy(&condition);
			mutex->release();
		}
};

class PosixSemaphoreData : public Semaphore::Data {
	public:
		PosixSemaphoreData(unsigned int initVal = 0) {
			if(sem_init(&mSemaphore, 0, initVal) < 0) throw runtime_error(string("sem_init: ") + strerror(errno));
		}

		void operator++() {
			sem_post(&mSemaphore);
		}

		void operator--() {
			sem_wait(&mSemaphore);
		}

		~PosixSemaphoreData() {
			sem_destroy(&mSemaphore);
		}
	private:
		sem_t mSemaphore;
};

Semaphore::Semaphore(unsigned int initVal) : mSemData(new PosixSemaphoreData(initVal))  {}
Semaphore::Data::~Data() {}

#ifdef ENABLE_CONDVAR
class PosixCondVarData : public CondVar::Data {
	public:
		PosixCondVarData() {
			pthread_cond_init(&mCondition, NULL);
			pthread_mutex_init(&mCondmutex, NULL);
		}
		void lock() {
			pthread_mutex_lock(&mCondmutex);
		}
		void unlock() {
			pthread_mutex_unlock(&mCondmutex);
		}
		void wait() {
			pthread_cond_wait(&mCondition, &mCondmutex);
		}
		void signal() {
			pthread_cond_signal(&mCondition);
		}
		~PosixCondVarData() {
			pthread_cond_destroy(&mCondition);
			pthread_mutex_destroy(&mCondmutex);
		}
	private:
		pthread_cond_t mCondition;
		pthread_mutex_t mCondmutex;
};

CondVar::CondVar() : mCVData(new PosixCondVarData()) {}
CondVar::Data::~Data() {}
#endif //ENABLE_CONDVAR

void delThread(void *thread) {
	linuxThreadData_t *t = (linuxThreadData_t *)thread;
	t->mutex->get();
	t->running = false;
	t->mutex->release();
	threadAboutToExit(t->thread);
	if(t->parent->autoDelete()) {
		delete t->parent;
		fprintf(stderr, "Thread deleted.\n");
	}
}

void *startThread(void *thread) {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGINT);
	sigaddset(&set, SIGTERM);
	// Can't fail - value is valid
	pthread_sigmask(SIG_BLOCK, &set, NULL);
	linuxThreadData_t *td = (linuxThreadData_t *)thread;
	td->running = true;
	td->paused = false;
	pthread_cleanup_push(&delThread, thread);
	td->parent->run();
	pthread_cleanup_pop(1);
	return NULL;
}

void thread_t::start() {

	threadData = auto_ptr<threadData_t>(new linuxThreadData_t());
	linuxThreadData_t &tdata = dynamic_cast<linuxThreadData_t &>(*threadData.get());
	tdata.mutex->get();
	tdata.parent = this;
	pthread_mutex_lock(&threadCountMutex);
	if(pthread_create(&tdata.thread, NULL, &startThread, (void *)&tdata)) {
		pthread_mutex_unlock(&threadCountMutex);
		throw runtime_error("Can't start thread");
	}
	++threadCount;
	pthread_mutex_unlock(&threadCountMutex);
	tdata.mutex->release();
}

void thread_t::pause() {
	linuxThreadData_t &tdata = dynamic_cast<linuxThreadData_t &>(*threadData.get());
	pthread_mutex_lock(&tdata.condmutex);
	tdata.paused = true;
	pthread_mutex_unlock(&tdata.condmutex);
}

void thread_t::resume() {
	linuxThreadData_t &tdata = dynamic_cast<linuxThreadData_t &>(*(threadData.get()));
	pthread_mutex_lock(&tdata.condmutex);
	tdata.paused = false;
	pthread_cond_signal(&tdata.condition);
	pthread_mutex_unlock(&tdata.condmutex);
}

bool thread_t::running() {
	linuxThreadData_t &tdata = dynamic_cast<linuxThreadData_t &>(*threadData.get());
	pthread_mutex_lock(&tdata.condmutex);
	bool paused = tdata.paused;
	pthread_mutex_unlock(&tdata.condmutex);
	return !paused;
}

void thread_t::pausePoint() {
	linuxThreadData_t &tdata = dynamic_cast<linuxThreadData_t &>(*threadData.get());
	pthread_mutex_lock(&tdata.condmutex);
	while(tdata.paused) pthread_cond_wait(&tdata.condition, &tdata.condmutex);
	pthread_mutex_unlock(&tdata.condmutex);
}

mutex_t::~mutex_t() {;}

threadData_t::~threadData_t() {;}

thread_t::~thread_t() {;}
