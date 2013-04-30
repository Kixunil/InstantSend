#include "serverlist.h"

void serverList_t::add(const string &name, const jsonComponent_t &pconf) {
	auto_ptr<connectionReceiver_t> cr(new connectionReceiver_t(name, pconf));
	servers[name].push_back(cr.get());
	cr.release()->start();
}

void serverList_t::remove(const string &name) {
	map<string, list<connectionReceiver_t *> >::iterator srv = servers.find(name);
	if(srv != servers.end()) remove(srv);
}

void serverList_t::remove() {
	for(map<string, list<connectionReceiver_t *> >::iterator it = servers.begin(); it != servers.end();) {
		/* Iterator increment can't be in for loop
		 * According to http://www.cplusplus.com/reference/map/map/erase/
		 * Iterators, pointers and references referring to elements removed
		 * by the function are invalidated.
		 * All other iterators, pointers and references keep their validity.
		 * it++ makes copy, so this is OK
		 */
		remove(it++);
	}
}

void serverList_t::remove(const map<string, list<connectionReceiver_t *> >::iterator &server) {
	for(list<connectionReceiver_t *>::iterator it = server->second.begin(); it != server->second.end(); ++it) {
		(*it)->stop();
	}
	servers.erase(server);
}

serverList_t &serverList_t::instance() {
	static serverList_t servlist;
	return servlist;
}
