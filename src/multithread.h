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

class Semaphore {
	public:
		Semaphore(unsigned int initVal = 0);

		inline void operator++() { ++*mSemData; }
		inline void operator--() { --*mSemData; }

		class Data {
			public:
				virtual void operator++() = 0;
				virtual void operator--() = 0;
				virtual ~Data();
		};

	private:
		auto_ptr<Semaphore::Data> mSemData;
		// copying disabled
		Semaphore(const Semaphore &);
		Semaphore &operator=(const Semaphore&);
};

class CondVar {
	public:
		CondVar();

		inline void lock() { mCVData->lock(); }
		inline void unlock() { mCVData->unlock(); }
		inline void wait() { mCVData->wait(); }
		inline void signal() { mCVData->signal(); }

		class Data {
			public:
				virtual void lock() = 0;
				virtual void unlock() = 0;
				virtual void wait() = 0;
				virtual void signal() = 0;
				virtual ~Data();
		};

	private:
		auto_ptr<CondVar::Data> mCVData;

		CondVar(const CondVar &);
		CondVar &operator=(const CondVar&);
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
