#ifndef MULTITHREAD
#define MULTITHREAD

#include <memory>

using namespace std;

class mutex_t {
	public:
		virtual void get() = 0;
		virtual void release() = 0;
		static auto_ptr<mutex_t> getNew();
		virtual ~mutex_t();
};

class threadData_t {
	public:
		virtual ~threadData_t();
};

class thread_t {
	private:
		auto_ptr<threadData_t> threadData;
	protected:
		void pausePoint();
	public:
		virtual void run() = 0;
		void start();
		void pause();
		void resume();
		bool running();
		virtual bool autoDelete() = 0;
		virtual ~thread_t();
};
#endif
