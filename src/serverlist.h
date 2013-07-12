#include <map>
#include <list>
#include <string>

#include "connectionreceiver.h"

namespace InstantSend {

class serverList_t {
	public:
		void add(const std::string &name, const jsonComponent_t &);
		void remove(const std::string &name);
		void remove();
		inline unsigned int count() { return servers.size(); }
		static serverList_t &instance();
	private:
		void remove(const map<std::string, list<ConnectionReceiver *> >::iterator &server);
		map<std::string, list<ConnectionReceiver *> > servers;
};

}
