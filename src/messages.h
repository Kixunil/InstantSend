#include <queue>
#include <vector>
#include <string>

#include "multithread.h"

using namespace std;

class messageTransmitter {
	public:
		virtual void pushMsg(string &msg) = 0;
};

class messageReceiver {
	public:
		virtual int count() = 0;
		virtual string popMsg() = 0;
};

class messageQueue_t : public messageTransmitter, messageReceiver {
	private:
		auto_ptr<mutex_t> mutex;
		queue<string> msgqueue;
	public:
		void pushMsg(string &msg);
		int count();
		string popMsg();

};
