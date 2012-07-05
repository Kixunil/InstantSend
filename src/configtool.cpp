#include <stdio.h>
#include <stdlib.h>
#include <memory>

#include "json.h"

using namespace std;

void printHelp() {
	printf("Arguments:\n\t--help, -h \t\tPrints this message\n\t--list-targets, -T\tPrints all targets\n\t--list-groups, -G\tPrints all groups\n\t--client, -C\t\tLoads client configuration from standard directory\n\t--server, -S\t\tLoads server configuration from standard directory\n\t--config, -c FILE\tLoads configuration from FILE\n");
}

void listTargets(jsonComponent_t *cfg) {
	jsonObj_t &targets = dynamic_cast<jsonObj_t &>(dynamic_cast<jsonObj_t &>(*cfg).gie("targets"));
	jsonIterator it = targets.begin(), end = targets.end();
	for(; it != end; ++it) {
		puts(it.key().c_str());
	}
}

void listGroups(jsonComponent_t *cfg) {
	jsonObj_t &targets = dynamic_cast<jsonObj_t &>(dynamic_cast<jsonObj_t &>(*cfg).gie("groups"));
	jsonIterator it = targets.begin(), end = targets.end();
	for(; it != end; ++it) {
		puts(it.key().c_str());
	}
}

int main(int argc, char **argv) {
	if(argc < 2) printHelp();
	auto_ptr<jsonComponent_t> cfg;
	char *homedir = getenv("HOME");
	for(int i = 1; i < argc; ++i) {
		if(string(argv[i]) == "--help" || string(argv[i]) == "-h") printHelp(); else
		if(string(argv[i]) == "--list-targets" || string(argv[i]) == "-T") listTargets(cfg.get()); else
		if(string(argv[i]) == "--list-groups" || string(argv[i]) == "-G") listGroups(cfg.get()); else
		if(string(argv[i]) == "--client" || string(argv[i]) == "-C") { 
			if(!homedir) {
				fprintf(stderr, "Config not found!\n");
				return 1;
			}
			cfg = cfgReadFile((string(homedir) + "/.instantsend/client.cfg").c_str());
		} else
		if(string(argv[i]) == "--server" || string(argv[i]) == "-S") {
			if(!homedir) {
				fprintf(stderr, "Config not found!\n");
				return 1;
			}
			cfg = cfgReadFile((string(homedir) + "/.instantsend/server.cfg").c_str());
		} else
		if(string(argv[i]) == "--config" || string(argv[i]) == "-c") {
			if(i + 1 >= argc) {
				fprintf(stderr, "Not enough arguments!\n");
				return 1;
			}
			cfg = cfgReadFile(argv[++i]);
		}
		
	}
	return 0;
}
