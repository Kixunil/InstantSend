#include <map>
#include <list>
#include <string>

#include "connectionreceiver.h"

using namespace std;

class serverList_t {
	public:
		void add(const string &name, const jsonComponent_t &);
		void remove(const string &name);
		void remove();
		inline unsigned int count() { return servers.size(); }
		static serverList_t &instance();
	private:
		void remove(const map<string, list<connectionReceiver_t *> >::iterator &server);
		map<string, list<connectionReceiver_t *> > servers;
};
