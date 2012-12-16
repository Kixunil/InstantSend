#include <stdio.h>
#include <stdlib.h>
#include <memory>

#include "json.h"
#include "sysapi.h"

using namespace std;

void printHelp() {
	printf("Arguments:\n"
			"\t--help, -h \t\tPrints this message\n"
			"\t--list-targets, -T\tPrints all targets\n"
			"\t--list-groups, -G\tPrints all groups\n"
			"\t--export-target, -Et\tPrints target configuration\n"
			"\t--add-targets, -At TARGETS\t\tAdds target(s) argument TARGETS have to be json object\n"
			"\t--add-ways, -Aw NAME WAYS\t\t Adds WAYS to target NAME. WAYS can be eighter json object (for one way) or json array of objects (for multiple ways)\n"
			"\t--import-targets, -It FILE\t\tImports target(s) from other configuration file\n"
			"\t--add-com-plugin, -Acp CONFIG\t\tAdds communication plugin for server. CONFIG have to be json object\n"
			"\t--init-server, -is\t\tDoesn't load configuration but creates empty instead. Use -S or -c FILE after this argument\n"
			"\t--init-client, -ic\t\tDoesn't load configuration but creates empty instead. Use -C or -c FILE after this argument\n"
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

void exportTarget(jsonComponent_t *cfg, const char *targname) {
	jsonObj_t &targets = dynamic_cast<jsonObj_t &>(dynamic_cast<jsonObj_t &>(*cfg).gie("targets"));
	jsonObj_t targconf;
	targconf[targname] = targets[targname];
	puts(targconf.toString().c_str());
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

void importTargets(jsonComponent_t *cfg, const char *filename) {
	auto_ptr<jsonComponent_t> other = cfgReadFile(filename);
	jsonObj_t &thesetargets = dynamic_cast<jsonObj_t &>(dynamic_cast<jsonObj_t &>(*cfg).gie("targets"));
	jsonObj_t &othertargets = dynamic_cast<jsonObj_t &>(dynamic_cast<jsonObj_t &>(*other.get()).gie("targets"));
	for(jsonIterator it = othertargets.begin(); it != othertargets.end(); ++it) {
		thesetargets.insertNew(it.key(), it.value()->clone());
	}
}

void addComPlugin(jsonComponent_t &cfg, const char *plugconf) {
	string pcfgstr(plugconf);
	jsonArr_t &cplugins = dynamic_cast<jsonArr_t &>(dynamic_cast<jsonObj_t &>(cfg).gie("complugins"));
	cplugins.addNew(new jsonObj_t(&pcfgstr));
}

void saveCfg(jsonComponent_t &cfg, string file) {
	string cfgstr = cfg.toString();
	FILE *tmp = fopen((file + "_tmp").c_str(), "w");
	if(!tmp) {
		perror("fopen");
		fprintf(stderr, "File: %s\n", (file + "_tmp").c_str());
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

	rename((file + "_tmp").c_str(), file.c_str());
}

void initClientCfg(auto_ptr<jsonComponent_t> &cfg) {
	auto_ptr<jsonObj_t> rootObj(new jsonObj_t());
	rootObj->insertNew("targets", new jsonObj_t());
	cfg = auto_ptr<jsonComponent_t>(rootObj.release());
}

void initServerCfg(auto_ptr<jsonComponent_t> &cfg) {
	cfg = auto_ptr<jsonComponent_t>(new jsonObj_t());
}

void changeCfg(auto_ptr<jsonComponent_t> &cfg, const string &newpath, string &oldpath, int &save, int &load) {
	if(load) {
		if(save) {
			saveCfg(*cfg.get(), oldpath);
			save = 0;             
		}
		cfg = cfgReadFile(newpath.c_str());
	}
	else {
		load = 1;
	}

	oldpath = newpath;
}

int main(int argc, char **argv) {
	if(argc < 2) printHelp();
	auto_ptr<jsonComponent_t> cfg;
	const char *homedir;
	try {
		homedir = getUserDir().c_str();
	} catch(exception &e) {
		fprintf(stderr, "Warning: Home directory (%s) not found!\n", e.what());
		homedir = NULL;
	}
	string cfgpath;
	int save = 0;
	int load = 1;
	for(int i = 1; i < argc; ++i) {
		if(string(argv[i]) == "--help" || string(argv[i]) == "-h") printHelp(); else
		if(string(argv[i]) == "--init-client" || string(argv[i]) == "-ic") {
			if(save) saveCfg(*cfg, cfgpath);
			initClientCfg(cfg);
			save = 1;
			load = 0;
		} else
		if(string(argv[i]) == "--init-server" || string(argv[i]) == "-is") {
			if(save) saveCfg(*cfg, cfgpath);
			initServerCfg(cfg);
			save = 1;
			load = 0;
		} else
		if(string(argv[i]) == "--list-targets" || string(argv[i]) == "-T") listTargets(cfg.get()); else
		if(string(argv[i]) == "--list-groups" || string(argv[i]) == "-G") listGroups(cfg.get()); else
		if(string(argv[i]) == "--export-target" || string(argv[i]) == "-Et") {
			if(i + 1 >= argc) {
				fprintf(stderr, "Not enough arguments!\n");
				return 1;
			}
			exportTarget(cfg.get(), argv[++i]);
		} else
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
		if(string(argv[i]) == "--import-targets" || string(argv[i]) == "-It") {
			if(i + 1 >= argc) {
				fprintf(stderr, "Not enough arguments!\n");
				return 1;
			}
			importTargets(cfg.get(), argv[++i]);
			save = 1;
		} else
		if(string(argv[i]) == "--add-com-plugin" || string(argv[i]) == "-Acp") {
			if(i + 1 >= argc) {
				fprintf(stderr, "Not enough arguments!\n");
				return 1;
			}
			addComPlugin(*cfg.get(), argv[++i]);
			save = 1;
		} else
		if(string(argv[i]) == "--client" || string(argv[i]) == "-C") { 
			if(!homedir) {
				fprintf(stderr, "Config not found!\n");
				return 1;
			}

			changeCfg(cfg, combinePath(string(homedir), "client.cfg"), cfgpath, save, load);
		} else
		if(string(argv[i]) == "--server" || string(argv[i]) == "-S") {
			if(!homedir) {
				fprintf(stderr, "Config not found!\n");
				return 1;
			}

			changeCfg(cfg, combinePath(string(homedir), "server.cfg"), cfgpath, save, load);
		} else
		if(string(argv[i]) == "--config" || string(argv[i]) == "-c") {
			if(i + 1 >= argc) {
				fprintf(stderr, "Not enough arguments!\n");
				return 1;
			}

			changeCfg(cfg, argv[++i], cfgpath, save, load);
		}
		
	}
	if(save) saveCfg(*cfg, cfgpath);
	return 0;
}
