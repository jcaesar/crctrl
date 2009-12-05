
#include "StringFunctions.h"

bool startswith(const std::string *str1, const std::string *str2){
	unsigned int len=str2->length();
	if(len > str1->length()) return 0;
	return (memcmp(str1->data(), str2->data(), len)==0);
}

bool startswith(const std::string *str1, const char *str2){
	unsigned int len=strlen(str2);
	if(len > str1->length()) return 0;
	return (memcmp(str1->data(), str2, len)==0);
}

bool startswith(const std::string *str1, const char str2_2){
	const char *str2; str2=&str2_2;
	unsigned int len=strlen(str2);
	if(len > str1->length()) return 0;
	return (memcmp(str1->data(), str2, len)==0);
}

bool nocasecmp(const char * str1, const char * str2){
	int diff;
	while(*str1 || *str2){
		diff = *str1 - *str2;
			if(diff!=0 && diff!=-32 && diff!=32) return false;
		str1++; str2++;
	}
	return true;
}

int strcharreplace(char * str, const char find, const char replace) {
	if(!str || !find || !replace) return -1;
	int ret = 0;
	while(*str) {
		if(*str==find) {*str=replace; ++ret;}
		++str;
	}
	return ret;
}
