#ifndef StringFunctionsH
#define StringFunctionsH

#include <string>
#include <string.h>

bool startswith(const std::string *str1, const std::string *str2);
bool startswith(const std::string *str1, const char *str2);
bool startswith(const std::string *str1, const char str2_2);
bool nocasecmp(const char * str1, const char * str2);
int strcharreplace(char * str, const char find, const char replace);

#endif