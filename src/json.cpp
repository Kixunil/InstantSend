#include <stdio.h>
#include <cmath>
#include <typeinfo>
#include <stdexcept>

#include "json.h"

#ifndef SECTOR_SIZE
	#define SECTOR_SIZE 4096
#endif

using namespace InstantSend;
using namespace std;

#ifdef NOPURE

string jsonComponent_t::toString() {
	throw runtime_error("Pure jsonComponent_t::toString called");
}

void jsonComponent_t::fromString(string *str) {
	throw runtime_error("Pure jsonComponent_t::fromString called");
}

jsonComponent_t *jsonComponent_t::clone() const {
	throw runtime_error("Pure jsonComponent_t::clone called");
}

#endif

namespace InstantSend {

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
	FILE *file = fopen(path, "rb");
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
	if(val == 0) return string("0");
	char buf[3*sizeof(intL_t) + 2];
	char *bptr = buf + 3*sizeof(intL_t) + 1;
	*bptr-- = 0;

	bool negative = false;
	intL_t tmp = val;
	if(tmp < 0) {
		tmp *= -1;
		negative = true;
	}

	while(tmp) {
		*bptr = '0' + (tmp % 10);
		--bptr;
		tmp /= 10;
	}
	if(negative) *bptr = '-'; else ++bptr;
	return string(bptr);
}

jsonComponent_t *jsonInt_t::clone() const {
	return new jsonInt_t(val);
}

void jsonInt_t::fromString(string *str) {
	setVal(parseInt(str));
}

jsonInt_t::~jsonInt_t() {}

// floatLeaf
jsonFloat_t::~jsonFloat_t() {}
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
#ifdef __APPLE__
	res *= pow((double)10, (int)expval);
#else
	res *= pow((double)10, expval);
#endif
	setVal(res);
}

string jsonFloat_t::toString() {
	char buf[50];

	snprintf(buf, 50, "%f", getVal());
	return string(buf);
}

jsonComponent_t *jsonFloat_t::clone() const {
	return new jsonFloat_t(val);
}

jsonComponent_t *jsonStr_t::clone() const {
	jsonStr_t *tmp = new jsonStr_t();
	tmp->setVal(val);
	return tmp;                      
}

// strLeaf
jsonStr_t::jsonStr_t() {
	setVal("");
}

jsonStr_t::~jsonStr_t() {}

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

jsonBool_t::~jsonBool_t() {}

jsonComponent_t *jsonBool_t::clone() const {
	return new jsonBool_t(val);
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

jsonNull_t::~jsonNull_t() {}

jsonComponent_t *jsonNull_t::clone() const {
	return new jsonNull_t();
}

/*
jsonStructuredComponent_t::~jsonStructuredComponent_t() {}
*/

// jsonArr_t
string jsonArr_t::toString() {
	string res = "[";
	for(unsigned int i = 0; i < data.size(); ++i) {
		res += data[i]->toString() + ", ";
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
		tmp->addNew(data[i]->clone());
	}
	return tmp;
}

jsonArr_t &jsonArr_t::operator=(const jsonArr_t &other) {
	if(this == &other) return *this;
	/*deleteContent();                

	for(unsigned int i = 0; i < other.data.size(); ++i) {
		if(other.data[i].isOwner()) addNew(other.data[i]->clone());
		else addVal(other.data[i]);
	}*/
	data = other.data;

	return *this;
}

void jsonArr_t::deleteContent() {
	data.clear();
}

jsonArr_t::~jsonArr_t() {
	//deleteContent(); // not needed, because vector automatically clears itself
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
		res += tmp.toString() + " : " + it->second->toString() + ", ";
	}


	if(res.length() > 1) res.erase(res.length() - 2, 2);
	res += "}";
	return res;
}

void jsonObj_t::insertNew(const string &key, jsonComponent_t *value) {
	pair<typeof(data.begin()), bool> inserted(data.insert(pair<string, itemContainer_t>(key, itemContainer_t(value, true))));
	if(!inserted.second) fprintf(stderr, "Warning: %s already exists!\n", key.c_str());
}

jsonComponent_t *jsonObj_t::clone() const {
	auto_ptr<jsonObj_t> tmp(new jsonObj_t());
	for(typeof(data.begin()) it = data.begin(); it != data.end(); ++it) {
		tmp->insertNew(it->first, it->second->clone());
	}
	return tmp.release();
}

jsonObj_t &jsonObj_t::operator=(const jsonObj_t &other) {
	if(this == &other) return *this;
/*	deleteContent();
	for(map<string, itemContainer_t>::const_iterator it = other.data.begin(); it != other.data.end(); ++it) {
		if(it->second.isOwner())
			data[it->first] = itemContainer_t(it->second->clone(), true);
		else data[it->first] = it->second;
	}*/
	data = other.data;
	return *this;
}

void jsonObj_t::deleteContent() {
	data.clear();
}

jsonObj_t::~jsonObj_t() {
	//deleteContent();
}

jsonNotExist::~jsonNotExist() throw() { ;}
fileNotAccesible::~fileNotAccesible() throw() {;}
fileUnreadable::~fileUnreadable() throw() {;}

}

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
