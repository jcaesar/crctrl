#ifdef unix
	#include <signal.h>
#endif
#include <iostream>
#include "helpers/AppManagement.h"
#include "helpers/ListenSocket.h"
#include "Config.h"
#include "Control.h"
#include "AutoHost.h"

#if (!defined WIN32) && (!defined unix)
	#error "I know, this comes late, but you're on the wrong system."
#endif

int main(int argc, char ** argv)
{
	#ifdef unix
		void CrashHandler(int);
		void EndHandler(int);
		void SigPipeHandler(int);
		signal(SIGBUS, CrashHandler);
		signal(SIGILL, CrashHandler);
		signal(SIGSEGV, CrashHandler);
		signal(SIGABRT, CrashHandler);
		signal(SIGQUIT, CrashHandler);
		signal(SIGFPE, CrashHandler);
		signal(SIGTERM, CrashHandler);
		signal(SIGINT, EndHandler);
		signal(SIGPIPE, SigPipeHandler);
	#elif defined WIN32
		void EndHandler();
		atexit(EndHandler);
	#endif

	char * farg = *argv;
	if(argc-- > 0) argv++;
	int create_autohost = 0; char * db = NULL, * usr = NULL, * pw = NULL, * addr = NULL;
	while(argc--){
		if(!strncmp(*argv, "db:", 3)) db=*argv+3;
		else if(!strncmp(*argv, "usr:", 4)) usr=*argv+4;
		else if(!strncmp(*argv, "pw:", 3)) pw=*argv+3;
		else if(!strncmp(*argv, "addr:", 5)) addr=*argv+5;
		else if(!strcmp(*argv, "auto")) create_autohost++;
		else {
			std::cerr << "Failing to parse \"" << *argv << "\"."  << std::endl;
			std::cerr << "Usage: " << farg << " db:<mysql-database> usr:<mysql-user> pw:<mysql-password> [auto]" << std::endl;
			std::cerr << "Example: " << farg << " db:clonk_cserv_database usr:cserv pw:cservs_password auto" << std::endl;
			exit(3);
		}
		argv++;
	}
	#if defined RUNTIME_LOGIN
		if(!db) std::cerr << "Error. No DB specified. Use db:<>"  << std::endl;
		if(!usr) std::cerr << "Error. No user specified. Use usr:<>"  << std::endl;
		if(!pw) std::cerr << "Warning. No password specified. Use pw:<>"  << std::endl;
		if(!db || !pw || !usr) exit(3);
	#endif
	GetConfig()->SetLoginData(usr,pw,db,addr);
	GetConfig()->Reload();
	while(create_autohost--) new AutoHost();
	Stream * io = new Stream();
	Stream::GetStandardIO(*io);
	new UserControl(io);
	ListenSocket queryport;
	if(!queryport.Init(GetConfig()->QueryPort)) {
		std::cerr << "Could not listen to " << GetConfig()->QueryPort << std::endl;
		while(true) Halt(60);
	}
	while(true) {
		Connection * c = new Connection; //Deleted in UserControl::~
		if(!queryport.AwaitConnection(c)) break;
		new UserControl(c);
	}
	while(true) Halt(60);
}

#ifdef unix

	void EndHandler(int signo){
		//delete &Games; What for?
		GetOut()->Put(NULL, "Exiting.", NULL);
		exit(0);
	}

	void SigPipeHandler(int signo){
		GetOut()->Put(NULL, "Got SIGPIPE... What now? :(", NULL);
	}

	void CrashHandler(int signo){
		std::cerr << "Exiting due to error. Signo #" << signo;
		switch(signo){
			case SIGBUS:  std::cerr << ": SIGBUS";  break;
			case SIGILL:  std::cerr << ": SIGILL";  break;
			case SIGSEGV: std::cerr << ": SIGSEGV"; break;
			case SIGABRT: std::cerr << ": SIGABRT"; break;
			case SIGQUIT: std::cerr << ": SIGQUIT"; break;
			case SIGFPE:  std::cerr << ": SIGFPE";  break;
			case SIGTERM: std::cerr << ": SIGTERM"; break;
		}
		std::cerr << std::endl;
		//exit(-1);
		PanicExit();
	}

#elif defined WIN32

	void EndHandler() {
		GetOut()->Put(NULL, "Exiting.", NULL);
	}

#endif