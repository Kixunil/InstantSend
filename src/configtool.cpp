#include <stdio.h>
#include <stdlib.h>
#include <memory>

#include "json.h"

using namespace std;

void printHelp() {
	printf("Arguments:\n"
			"\t--help, -h \t\tPrints this message\n"
			"\t--list-targets, -T\tPrints all targets\n"
			"\t--list-groups, -G\tPrints all groups\n"
			"\t--add-targets, -At TARGETS\t\tAdds target(s) argument TARGETS have to be json object\n"
			"\t--add-ways, -Aw NAME WAYS\t\t Adds WAYS to target NAME. WAYS can be eighter json object (for one way) or json array of objects (for multiple ways)\n"
			"\t--client, -C\t\tLoads client configuration from standard directory\n"
			"\t--server, -S\t\tLoads server configuration from standard directory\n"
			"\t--config, -c FILE\tLoads configuration from FILE\n");
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

void addTargets(jsonComponent_t *cfg, const char *targconf) {
	string tcfgstr(targconf);
	jsonObj_t tcfg(&tcfgstr);
	jsonObj_t &targets = dynamic_cast<jsonObj_t &>(dynamic_cast<jsonObj_t &>(*cfg).gie("targets"));
	jsonIterator it = tcfg.begin(), end = tcfg.end();
	for(; it != end; ++it) {
		if(targets.exists(it.key())) {
			printf("Target %s already exists. Use -Ut to update it", it.key().c_str());
			continue;
		}
		targets.insertNew(it.key(), it.value()->clone()); 
	}
}

void addWays(jsonComponent_t &cfg, const char *target, const char *waycfg) {
	jsonObj_t &targets = dynamic_cast<jsonObj_t &>(dynamic_cast<jsonObj_t &>(cfg).gie("targets"));
	jsonArr_t &ways = dynamic_cast<jsonArr_t &>(dynamic_cast<jsonObj_t &>(targets.gie(target)).gie("ways"));
	auto_ptr<jsonComponent_t> wcfg(cfgReadStr(waycfg));
	jsonArr_t *warr;
	if((warr = dynamic_cast<jsonArr_t *>(wcfg.get()))) {
		for(int i = 0; i < warr->count(); ++i) {
			ways.addNew((*warr)[i]->clone());
		}
	} else {
		 ways.addNew(wcfg.release());
	}
}

void saveCfg(jsonComponent_t &cfg, string file) {
	string cfgstr = cfg.toString();
	FILE *tmp = fopen(string(file + "_tmp").c_str(), "w");
	if(!tmp) {
		perror("fopen");
		return;
	}

	if(fwrite(cfgstr.c_str(), cfgstr.size() * sizeof(char), 1, tmp) < 1) {
		perror("fwrite");
		return;
	}

	if(fclose(tmp) != 0) {
		perror("fwrite");
		return;
	}

	rename(string(file + "_tmp").c_str(), file.c_str());
}

int main(int argc, char **argv) {
	if(argc < 2) printHelp();
	auto_ptr<jsonComponent_t> cfg;
	char *homedir = getenv("HOME");
	string cfgpath;
	int save = 0;
	for(int i = 1; i < argc; ++i) {
		if(string(argv[i]) == "--help" || string(argv[i]) == "-h") printHelp(); else
		if(string(argv[i]) == "--list-targets" || string(argv[i]) == "-T") listTargets(cfg.get()); else
		if(string(argv[i]) == "--list-groups" || string(argv[i]) == "-G") listGroups(cfg.get()); else
		if(string(argv[i]) == "--add-targets" || string(argv[i]) == "-At") {
			if(i + 1 >= argc) {
				fprintf(stderr, "Not enough arguments!\n");
				return 1;
			}
			addTargets(cfg.get(), argv[++i]);
			save = 1;
		} else
		if(string(argv[i]) == "--add-way" || string(argv[i]) == "-Aw") {
			if(i + 2 >= argc) {
				fprintf(stderr, "Not enough arguments!\n");
				return 1; 
			}
			addWays(*cfg.get(), argv[i+1], argv[i+2]);
			save = 1;
			i += 2;
		} else
		if(string(argv[i]) == "--client" || string(argv[i]) == "-C") { 
			if(!homedir) {
				fprintf(stderr, "Config not found!\n");
				return 1;
			}
			if(save) {
				saveCfg(*cfg.get(), cfgpath);
				save = 0;             
			}
			cfgpath = string(homedir) + "/.instantsend/client.cfg";
			cfg = cfgReadFile(cfgpath.c_str());
		} else
		if(string(argv[i]) == "--server" || string(argv[i]) == "-S") {
			if(!homedir) {
				fprintf(stderr, "Config not found!\n");
				return 1;
			}
			if(save) {
				saveCfg(*cfg.get(), cfgpath);
				save = 0;
			}
			cfgpath = string(homedir) + "/.instantsend/server.cfg";
			cfg = cfgReadFile(cfgpath.c_str());                    
		} else
		if(string(argv[i]) == "--config" || string(argv[i]) == "-c") {
			if(i + 1 >= argc) {
				fprintf(stderr, "Not enough arguments!\n");
				return 1;
			}
			if(save) {
				saveCfg(*cfg.get(), cfgpath);
				save = 0;             
			}
			cfgpath = argv[++i];
			cfg = cfgReadFile(cfgpath.c_str());                    
		}
		
	}
	if(save) saveCfg(*cfg.get(), cfgpath);
	return 0;
}
