#include <stdio.h>
#include <pthread.h>
#include <memory>
#include <stdexcept>
#include <string.h>

#include "multithread.h"

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

void delThread(void *thread) {
	linuxThreadData_t *t = (linuxThreadData_t *)thread;
	t->mutex->get();
	t->running = false;
	t->mutex->release();
	if(t->parent->autoDelete()) delete t->parent;
}

void *startThread(void *thread) {
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
	if(pthread_create(&tdata.thread, NULL, &startThread, (void *)&tdata)) {
		throw runtime_error("Can't start thread");
	}
	if(autoDelete()) pthread_detach(tdata.thread);
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
