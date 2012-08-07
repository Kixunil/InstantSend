#include <stdio.h>
#include <pthread.h>
#include <memory>
#include <stdexcept>

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
	public:
		pthread_t thread;
		thread_t *parent;
		//pthread_attr_t threadAttr;
		auto_ptr<mutex_t> mutex;
		bool running;

		linuxThreadData_t() {
			mutex = mutex_t::getNew();
		}

		~linuxThreadData_t() {
			mutex->get();
			if(running) pthread_exit(&thread);
			mutex->release();
		}
};

void delThread(void *thread) {
	linuxThreadData_t *t = (linuxThreadData_t *)thread;
	t->mutex->get();
	t->running = false;
	t->mutex->release();
	delete t->parent;
}

void *startThread(void *thread) {
	linuxThreadData_t *td = (linuxThreadData_t *)thread;
	if(td->parent->autoDelete()) {
		pthread_cleanup_push(&delThread, thread);
		td->parent->run();
		pthread_cleanup_pop(1);
	} else td->parent->run();
	return NULL;
}

void thread_t::start() {

	threadData = auto_ptr<threadData_t>(new linuxThreadData_t());
	linuxThreadData_t *tdata = dynamic_cast<linuxThreadData_t *>(threadData.get());
	tdata->mutex->get();
	tdata->parent = this;
	if(pthread_create(&tdata->thread, NULL, &startThread, (void *)tdata)) {
		throw runtime_error("Can't start thread");
	}
	if(autoDelete()) pthread_detach(tdata->thread);
	tdata->mutex->release();
}

mutex_t::~mutex_t() {;}

threadData_t::~threadData_t() {;}

thread_t::~thread_t() {;}
