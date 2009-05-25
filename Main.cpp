//#include "Umlaut.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <boost/regex.hpp>
#include <mysql++.h>
#include "Lib.cpp"
#include "Config.h"
#include "Config.cpp"
#include "GameControl.h"
#include "AutoHost.h"
#include "GameControl.cpp"
#include "Control.h"
#include "Control.cpp"
#define DEFAULT_SQL_PW "sLZpTCMMHZmnvebA"

void CrashHandler(int);
void EndHandler(int);
void SigPipeHandler(int);

int main(int argc, char ** argv)
{
	signal(SIGBUS, CrashHandler);
	signal(SIGILL, CrashHandler);
	signal(SIGSEGV, CrashHandler);
	signal(SIGABRT, CrashHandler);
	signal(SIGQUIT, CrashHandler);
	signal(SIGFPE, CrashHandler);
	signal(SIGTERM, CrashHandler);
	signal(SIGINT, EndHandler);
	signal(SIGPIPE, SigPipeHandler);
	
	if(argc-- > 0) argv++;
	bool create_autohost = false; char * db = NULL, * usr = NULL, * pw = NULL, * addr = NULL;
	while(argc--){
		if(!strncmp(*argv, "db:", 3)) db=*argv+3;
		else if(!strncmp(*argv, "usr:", 4)) usr=*argv+4;
		else if(!strncmp(*argv, "pw:", 3)) pw=*argv+3;
		else if(!strncmp(*argv, "addr:", 5)) addr=*argv+5;
		else if(!strcmp(*argv, "auto")) create_autohost = true;
		argv++;
	}
	if(usr == 0) usr="crctrl";
	if(pw == 0) pw=DEFAULT_SQL_PW;
	if(db == 0) db="crctrl";
	Config.Reload(usr,pw,db,addr);
	if(strcmp(pw, DEFAULT_SQL_PW)) while(*pw != 0) *pw++=NULL; //Eleminate the pw from ram by overwriting it. (But don't overwrite the default.)
	if(create_autohost) new AutoHost();
	new StreamControl(STDIN_FILENO,STDOUT_FILENO);
	while(true) sleep(10);
}

void EndHandler(int signo){
	//delete &Games; What for?
	Out.Put("Exiting.", NULL);
	exit(0);
}

void SigPipeHandler(int signo){
	Out.Put("Got SIGPIPE... What now? :(", NULL);
}

void CrashHandler(int signo){
	std::cerr << "Exiting due to error. Signo #" << signo << ": ";
	switch(signo){
		case SIGBUS: std::cerr << "SIGBUS"; break;
		case SIGILL: std::cerr << "SIGILL"; break;
		case SIGSEGV: std::cerr << "SIGSEGV"; break;
		case SIGABRT: std::cerr << "SIGABRT"; break;
		case SIGQUIT: std::cerr << "SIGQUIT"; break;
		case SIGFPE: std::cerr << "SIGFPE"; break;
		case SIGTERM: std::cerr << "SIGTERM"; break;
	}
	std::cerr << std::endl;
	//exit(-1);
	raise(SIGKILL);
}