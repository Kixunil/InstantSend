#include <stdio.h>
#include <cmath>
#include <typeinfo>
#include <stdexcept>

#include "json.h"

#ifndef SECTOR_SIZE
	#define SECTOR_SIZE 4096
#endif

jsonSyntaxErr::~jsonSyntaxErr() throw() { ; }

jsonComponent_t *loadComponent(string *str) {
	switch((*str)[0]) {
		case '{': // }
			return new jsonObj_t(str);
		case '[': // ]
			return new jsonArr_t(str);
		case 't':
		case 'f':
			return new jsonBool_t(str);
		case 'n':
			return new jsonNull_t(str);
		case '"':
			return new jsonStr_t(str);
		case '-':
		case '0'...'9':
			jsonInt_t *intVal = new jsonInt_t(str);
			if((*str)[0] != '.') return intVal;
			jsonFloat_t *floatVal = new jsonFloat_t(str);
			if(intVal < 0) floatVal->setVal(intVal->getVal() - floatVal->getVal()); else floatVal->setVal(floatVal->getVal() + intVal->getVal());
			delete intVal;
			return floatVal;
	}
	throw jsonSyntaxErr(str, 0);
}

// Wrapper functions

auto_ptr<jsonComponent_t> cfgReadStr(const char *str) {
	string cppstr = string(str);
	return auto_ptr<jsonComponent_t>(loadComponent(&cppstr));
}

auto_ptr<jsonComponent_t> cfgReadFile(const char *path) {
	string str;
	FILE *file = fopen(path, "r");
	ssize_t len;
	char buf[SECTOR_SIZE + 1];

	if(file == NULL) throw runtime_error((string("Can't access file ") + path).c_str()); //fileNotAccesible(string(path));
	if(fseek(file, 0, SEEK_END) == 0) {
		str.reserve(ftell(file));
		rewind(file);
	}

	while((len = fread(buf, 1, SECTOR_SIZE, file)) > 0) {
		buf[len] = 0;
		str += buf;
	}

	fclose(file);

	if(len < 0) throw fileUnreadable(string(path));

	return auto_ptr<jsonComponent_t>(loadComponent(&str));
}

jsonComponent_t::~jsonComponent_t() {;}
/*
int cfgIsObj(jsonComponent_t *config) {
	return typeid(*config) == typeid(jsonObj_t);
}

int cfgIsArr(jsonComponent_t *config) {
	return typeid(*config) == typeid(jsonArr_t);
}

int cfgIsInt(jsonComponent_t *config) {
	return typeid(*config) == typeid(jsonInt_t);
}

int cfgIsFloat(jsonComponent_t *config) {
	return typeid(*config) == typeid(jsonFloat_t);
}

int cfgIsNum(jsonComponent_t *config) {
	return typeid(*config) == typeid(jsonInt_t) || typeid(*config) == typeid(jsonFloat_t);
}

int cfgIsBool(jsonComponent_t *config) {
	return typeid(*config) == typeid(jsonBool_t);
}

int cfgIsNull(jsonComponent_t *config) {
	return typeid(*config) == typeid(jsonNull_t);
}

int cfgIsStr(jsonComponent_t *config) {
	return typeid(*config) == typeid(jsonStr_t);
}

intL_t cfgGetValInt(jsonInt_t *config) {
	return config->getVal();
}

floatL_t cfgGetValFloat(jsonComponent_t *config) {
	if(cfgIsInt(config)) return ((jsonInt_t *)config)->getVal(); else return ((jsonFloat_t *)config)->getVal();
}

int cfgGetValBool(jsonBool_t *config) {
	return config->getVal();
}

const char *cfgGetValStr(jsonStr_t *config) {
	return config->getVal().c_str();
}

jsonComponent_t *cfgGetChild(jsonObj_t *config, const char *key) {
	string keystr = string(key);
	return (*config)[keystr];
}

jsonComponent_t *cfgGetChildIfExists(jsonObj_t *config, const char *key) {
	string keystr = string(key);
	if(config->exists(string(key))) return (*config)[keystr]; else return NULL;
}

jsonComponent_t *cfgGetItem(jsonArr_t *config, int ptr) {
	return (*config)[ptr];
}

int cfgIsEmpty(jsonComponent_t *config) {
	if(cfgIsArr(config)) return ((jsonArr_t *)config)->empty(); else return ((jsonObj_t *)config)->empty();
}

int cfgChildExists(jsonObj_t *config, const char *key) {
	return config->exists(string(key));
}

int cfgItemCount(jsonComponent_t *config) {
	if(cfgIsArr(config)) return ((jsonArr_t *)config)->count(); else return ((jsonObj_t *)config)->count();
}
*/
// End of wrapper functions

intL_t parseInt(string *str) {
	intL_t value = 0;
	unsigned int i = 0;
	if((*str)[0] == '-' || (*str)[0] == '+') ++i;
	for(; i < str->length() && (*str)[i] >= '0' && (*str)[i] <= '9'; ++i) {
		value *= 10;
		value += (*str)[i] - '0';
	}
	if((*str)[0] == '-') value = -value;


	str->erase(0, i);
	return value;
}

// intLeaf
/*jsonInt_t::jsonInt_t(intL_t value) {
	setVal(value);
}*/

jsonInt_t::jsonInt_t(string *str) {
	fromString(str);
}

string jsonInt_t::toString() {
	char buf[10];

	snprintf(buf, 10, "%d", getVal());
	return string(buf);
}

void jsonInt_t::fromString(string *str) {
	setVal(parseInt(str));
}

// floatLeaf
jsonFloat_t::jsonFloat_t(floatL_t value) {
	setVal(value);
}

jsonFloat_t::jsonFloat_t(string *str) {
	fromString(str);
}

void jsonFloat_t::fromString(string *str) {
	intL_t intval = 0, decval = 0, expval;
	floatL_t res;
	if((*str)[0] != '.') intval = parseInt(str);
	if(str->empty() || (*str)[0] != '.') {
		setVal(intval);
		return;
	}
	unsigned int i;
	int div = 1;
	for(i = 1; i < str->length() && (*str)[i] >= '0' && (*str)[i] <= '9'; ++i, div *= 10) {
		decval *= 10;
		decval += (*str)[i] - '0';
	}
	if(intval >=0) res = intval + (floatL_t)decval / div; else res = intval - (floatL_t)decval / div;

	str->erase(0, i);
	if(str->empty() || ((*str)[0] != 'e' && (*str)[0] != 'E')) {
		setVal(res);
		return;
	}


	str->erase(0, 1);
	expval = parseInt(str);
	res *= pow(10, expval);
	setVal(res);
}

string jsonFloat_t::toString() {
	char buf[50];

	snprintf(buf, 50, "%f", getVal());
	return string(buf);
}

// strLeaf
jsonStr_t::jsonStr_t() {
	setVal("");
}

jsonStr_t::jsonStr_t(string *str) {
	fromString(str);
}

unsigned int unicodeHexToInt(string &str) {
	if(str.length() != 4) throw jsonSyntaxErr(&str, str.length() - 1);
	int i;
	unsigned int res = 0;
	for(i = 0; i < 4; ++i) {
		res *= 16;
		if(str[i] >= '0' && str[i] <= '9') res += str[i] - '0'; else
		if(str[i] >= 'a' && str[i] <= 'f') res += str[i] - 'a' + 10; else
		if(str[i] >= 'A' && str[i] <= 'F') res += str[i] - 'A' + 10; // else throw ...
	}
	return res;
}

// taken from jsoncpp, which is under public domain
string codePointToUTF8(unsigned int cp) {
   std::string result;
   
   // based on description from http://en.wikipedia.org/wiki/UTF-8

   if (cp <= 0x7f) 
   {
      result.resize(1);
      result[0] = static_cast<char>(cp);
   } 
   else if (cp <= 0x7FF) 
   {
      result.resize(2);
      result[1] = static_cast<char>(0x80 | (0x3f & cp));
      result[0] = static_cast<char>(0xC0 | (0x1f & (cp >> 6)));
   } 
   else if (cp <= 0xFFFF) 
   {
      result.resize(3);
      result[2] = static_cast<char>(0x80 | (0x3f & cp));
      result[1] = 0x80 | static_cast<char>((0x3f & (cp >> 6)));
      result[0] = 0xE0 | static_cast<char>((0xf & (cp >> 12)));
   }
   else if (cp <= 0x10FFFF) 
   {
      result.resize(4);
      result[3] = static_cast<char>(0x80 | (0x3f & cp));
      result[2] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
      result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 12)));
      result[0] = static_cast<char>(0xF0 | (0x7 & (cp >> 18)));
   }

   return result;
}

void jsonStr_t::fromString(string *str) {
	if(str->empty() || (*str)[0] != '"') throw jsonSyntaxErr(str, 0);
	unsigned int i;
	string res = "";

	for(i = 1; i < str->length() && (*str)[i] != '"' && (*str)[i] != 0; ++i) {
		// if((*str)[i] < 32) throw
		if((*str)[i] == '\\' && i + 1 < str->length()) {
			switch((*str)[i + 1]) {
				case '"':
					res += "\"";
					break;
				case '\\':
					res += "\\";
					break;
				case '/':
					res += "/";
					break;
				case 'b':
					res += "\b";
					break;
				case 'f':
					res += "\f";
					break;
				case 'n':
					res += "\n";
					break;
				case 'r':
					res += "\r";
					break;
				case 't':
					res += "\t";
					break;
				case 'u':
					string tmp = str->substr(i + 2, 4);
					res += codePointToUTF8(unicodeHexToInt(tmp));
					i += 4;
					break;
			}
			++i;
			continue;
		} else res += (*str)[i];
	}
	if(i >= str->length()) throw jsonSyntaxErr(str, str->length() - 1);
	setVal(res);


	str->erase(0, i+1);
}

string jsonStr_t::toString() {
	string res = "\"", in = getVal();
	res.reserve(in.length() * 2 + 3);
	for(unsigned int i = 0; i < in.length(); ++i) {
		switch(in[i]) {
			case '\\':
				res += "\\\\";
				break;
			case '"':
				res += "\\\"";
				break;
			case '/':
				res += "\\/";
				break;
			case '\b':
				res += "\\b";
				break;
			case '\n':
				res += "\\n";
				break;
			case '\f':
				res += "\\f";
				break;
			case '\r':
				res += "\\r";
				break;
			case '\t':
				res += "\\t";
				break;
			default:
				res += in[i];
		}
	}
	res += "\"";
	return res;
}

jsonBool_t::jsonBool_t(bool value) {
	setVal(value);
}

jsonBool_t::jsonBool_t(string *str) {
	fromString(str);
}

void jsonBool_t::fromString(string *str) {
	if(str->substr(0, 4) == "true") {
		setVal(true);


		str->erase(0, 4);
	} else if(str->substr(0, 5) == "false") {
		setVal(false);


		str->erase(0, 5);
	} else throw jsonSyntaxErr(str, 0);
}

string jsonBool_t::toString() {
	if(getVal()) return "true"; else return "false";
}

// jsonArr_t
string jsonArr_t::toString() {
	string res = "[";
	for(unsigned int i = 0; i < data.size(); ++i) {
		res += data[i].item->toString() + ", ";
	}


	if(res.length() > 1) res.erase(res.length() - 2, 2);
	res += "]";
	return res;
}

void jsonArr_t::fromString(string *str) {
	if((*str)[0] != '[') throw jsonSyntaxErr(str, 0);
	if(!data.empty()) deleteContent();
	unsigned int i = 1;
	do {
		for(; i < str->length() && ((*str)[i] == ' ' || (*str)[i] == '\n' || (*str)[i] == '\t' || (*str)[i] == ','); ++i);

		if((*str)[i] == ']') {
			str->erase(0, i + 1);
			break;
		} else str->erase(0, i);
		//printf("'%s'\n", str->c_str());
		data.push_back(itemContainer_t(loadComponent(str), true));
		i = 0;
	} while(str->length() > 0);
}

jsonComponent_t *jsonArr_t::clone() const {
	jsonArr_t *tmp = new jsonArr_t();
	for(unsigned int i = 0; i < data.size(); ++i) {
		tmp->data[i] = itemContainer_t(data[i].item->clone(), true);
	}
	return tmp;
}

jsonArr_t &jsonArr_t::operator=(const jsonArr_t &other) {
	if(this == &other) return *this;
	deleteContent();                

	for(unsigned int i = 0; i < other.data.size(); ++i) {
		if(other.data[i].isOwner()) data[i] = itemContainer_t(other.data[i].item->clone(), true);
		else data[i] = other.data[i];
	}

	return *this;
}

void jsonArr_t::deleteContent() {
	for(unsigned int i = 0; i < data.size(); ++i) {
		if(data[i].isOwner()) delete data[i].item;
	}

	data.clear();
}

jsonArr_t::~jsonArr_t() {
	deleteContent();
}

// jsonObj_t - JSON object
void jsonObj_t::fromString(string *str) {
	if((*str)[0] != '{') throw jsonSyntaxErr(str, 0);
	string key;
	unsigned int i = 1, m;
	do {
		for(m = str->length(); i < m && ((*str)[i] == ' ' || (*str)[i] == '\n' || (*str)[i] == '\t' || (*str)[i] == ','); ++i);

		if((*str)[i] == '}') {
			break;
		} else str->erase(0, i);
		key = jsonStr_t(str).getVal();
		/*printf("%s\n", str->c_str());
		fflush(stderr);*/

		for(i = 0; i < str->length() && ((*str)[i] == ' ' || (*str)[i] == '\n' || (*str)[i] == '\t'); ++i);
		if((*str)[i] != ':') throw jsonSyntaxErr(str, i);
		for(++i; i < str->length() && ((*str)[i] == ' ' || (*str)[i] == '\n' || (*str)[i] == '\t'); ++i);
		str->erase(0, i);
		//printf("%s\n", str->c_str());
		data.insert(pair<string, itemContainer_t>(key, itemContainer_t(loadComponent(str), true)));
		//printf("'%s'\n", str->c_str());

		i = 0;
	} while(str->length());
	if(str->length()) str->erase(0, i + 1); else throw jsonSyntaxErr(str, -1);
}

string jsonObj_t::toString() {
	string res = "{";
	jsonStr_t tmp;
	for (typeof(data.begin()) it = data.begin(); it != data.end(); ++it) {
		tmp.setVal(it->first);
		res += tmp.toString() + " : " + (it->second).item->toString() + ", ";
	}


	if(res.length() > 1) res.erase(res.length() - 2, 2);
	res += "}";
	return res;
}

void jsonObj_t::insertNew(const string &key, jsonComponent_t *value) {
	itemContainer_t &container = data[key];
	if(container.isOwner()) delete container.item;
	container = itemContainer_t(value, true);
}

jsonComponent_t *jsonObj_t::clone() const {
	jsonObj_t *tmp = new jsonObj_t();
	for(typeof(data.begin()) it = data.begin(); it != data.end(); ++it) {
		tmp->data[it->first] = itemContainer_t(it->second.item->clone(), true);
	}
	return tmp;
}

jsonObj_t &jsonObj_t::operator=(const jsonObj_t &other) {
	if(this == &other) return *this;
	deleteContent();
	for(map<string, itemContainer_t>::const_iterator it = other.data.begin(); it != other.data.end(); ++it) {
		if(it->second.isOwner()) data[it->first] = itemContainer_t(it->second.item->clone(), true);
		else data[it->first] = it->second;
	}
	return *this;
}

void jsonObj_t::deleteContent() {
	for(typeof(data.begin()) it = data.begin(); it != data.end(); ++it) {
		if(it->second.isOwner()) delete it->second.item;
	}

	data.clear();
}

jsonObj_t::~jsonObj_t() {
	deleteContent();
}

jsonNotExist::~jsonNotExist() throw() { ;}
fileNotAccesible::~fileNotAccesible() throw() {;}
fileUnreadable::~fileUnreadable() throw() {;}

// JAL
/*
auto_ptr<JALParser_t> parseJAL(const string &str) {
	if(str[0] == '.') return auto_ptr<JALParser_t>(new JALObjParser_t(str)); else
	if(str[0] == '[') return auto_ptr<JALParser_t>(new JALArrParser_t(str)); else
	throw jsonSyntaxErr(string("Unknown token: '") + str[0] + '\'');
}

JALParser_t::JALParser_t(const string &str) {
	string tmp = str;
	subparser = parseJAL(tmp);
}

jsonComponent_t *&JALParser_t::access(jsonComponent_t &json) {
	return subparser->access(json);
}

JALObjParser_t::JALObjParser_t(const string &str) {
	if(str[0] != '.') throw jsonSyntaxErr(string("Unknown token: '") + str[0] + '\'');
	string tmp = str.substr(1);
	key = jsonStr_t(&tmp).getVal();
	if(tmp.size() > 0) subparser = parseJAL(tmp);
}

jsonComponent_t *&JALObjParser_t::access(jsonComponent_t &json) {
	jsonComponent_t *&child = dynamic_cast<jsonObj_t &>(json)[key];
	if(subparser.get() && child) return subparser->access(*child);
	return child;
}

JALArrParser_t::JALArrParser_t(const string &str) {
	if(str[0] != '[') throw jsonSyntaxErr(string("Unknown token: '") + str[0] + '\'');
	string tmp = str.substr(1);
	index = jsonInt_t(&tmp).getVal();
	if(tmp[0] != ']') throw jsonSyntaxErr(string("Unknown token: '") + tmp[0] + '\'');
	if(tmp.size() > 1) {
		tmp.erase(0, 1);
		subparser = parseJAL(tmp);
	}
}

jsonComponent_t *&JALArrParser_t::access(jsonComponent_t &json) {
	jsonComponent_t *&child = dynamic_cast<jsonArr_t &>(json)[index];
	if(subparser.get()) return subparser->access(*child);
	return child;
}*/
