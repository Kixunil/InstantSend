#include <string>
#include <unistd.h>
#include <memory>

#include "json.h"

using namespace std;

int main() {
	string input;
	char buf[1025];
	ssize_t len;
	while((len = read(0, buf, 1024)) > 0) {
		buf[len] = 0;
		input += buf;
	}

	auto_ptr<jsonComponent_t> data = cfgReadStr(input.c_str());
	jsonArr_t &meals = dynamic_cast<jsonArr_t &>(*data.get());
	for(int i = 0; i < meals.count(); ++i) {
		jsonObj_t &meal = dynamic_cast<jsonObj_t &>(*meals[i]);
		if(dynamic_cast<jsonInt_t *>(meal["canteen_id"])->getVal() == 1)
	}
}
