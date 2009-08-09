#include <signal.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "Config.h"
#include "Control.h"
#include "AutoHost.h"


void CrashHandler(int);
void EndHandler(int);
void SigPipeHandler(int);

int main(int argc, char* *argv)
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
	int create_autohost = 0; char * db = NULL, * usr = NULL, * pw = NULL, * addr = NULL;
	while(argc--){
		if(!strncmp(*argv, "db:", 3)) db=*argv+3;
		else if(!strncmp(*argv, "usr:", 4)) usr=*argv+4;
		else if(!strncmp(*argv, "pw:", 3)) pw=*argv+3;
		else if(!strncmp(*argv, "addr:", 5)) addr=*argv+5;
		else if(!strcmp(*argv, "auto")) create_autohost++;
		else {
			std::cerr << printf("Usage: %s %s %s %s %s", *argv, "db:<mysql-database>", "usr:<mysql-user>", "pw:<mysql-password>", "[auto]") << std::endl;
			std::cerr << printf("Example: %s db:clonk_cserv_database usr:cserv pw:cservs_password auto", *argv);
			exit(3);
		}
		argv++;
	}
	GetConfig()->SetLoginData(usr,pw,db,addr);
	GetConfig()->Reload();
	while(create_autohost--) new AutoHost();
	new StreamControl(STDIN_FILENO,STDOUT_FILENO);
	
	int       list_s;                /*  listening socket          */
    int       conn_s;                /*  connection socket         */
    struct    sockaddr_in servaddr;  /*  socket address structure  */
	#define Deathloop while(true) sleep(100);
    if ((list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
		std::cerr << "Error creating listening socket.\n";
		Deathloop
    }
	memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(GetConfig()->QueryPort);
    if (bind(list_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
		std::cerr << "Could not bind to socket.\n";
		Deathloop
    }
    if (listen(list_s, 8) < 0 ) {
		std::cerr << "Could not listen to socket.\n";
		Deathloop
    }
    while (1) {
		if ((conn_s = accept(list_s, NULL, NULL) ) < 0 ) {
			std::cerr << "Could not accept connection.\n";
			break;
		}
		new StreamControl(conn_s, conn_s);
    }
	Deathloop
}

void EndHandler(int signo){
	//delete &Games; What for?
	GetOut()->Put(NULL, "Exiting.", NULL);
	exit(0);
}

void SigPipeHandler(int signo){
	GetOut()->Put(NULL, "Got SIGPIPE... What now? :(", NULL);
}

void CrashHandler(int signo){
	std::cerr << "Exiting due to error. Signo #" << signo << ": ";
	switch(signo){
		case SIGBUS:  std::cerr << "SIGBUS";  break;
		case SIGILL:  std::cerr << "SIGILL";  break;
		case SIGSEGV: std::cerr << "SIGSEGV"; break;
		case SIGABRT: std::cerr << "SIGABRT"; break;
		case SIGQUIT: std::cerr << "SIGQUIT"; break;
		case SIGFPE:  std::cerr << "SIGFPE";  break;
		case SIGTERM: std::cerr << "SIGTERM"; break;
	}
	std::cerr << std::endl;
	//exit(-1);
	raise(SIGKILL);
}
