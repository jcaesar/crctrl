
#include "AppManagement.h"

#include <math.h>
#include <iostream>
#if defined unix
	#include <unistd.h>
	#include <errno.h>
	#include <string.h>
	#include <stdarg.h>
	#include <signal.h>
	#include <fcntl.h>
	#include <sys/stat.h>
#elif defined WIN32
	#include <windows.h>
	//#include <stdlib.h>
#endif

void Halt(double t) {
	#ifdef unix
		sleep(t);
		usleep(fmod(t,1));
	#elif defined WIN32
		Sleep((DWORD)t*1000);
	#endif
}

void PanicExit(){
	#if defined unix
		raise(SIGKILL);
	#elif defined WIN32
		TerminateProcess(GetCurrentProcess(), 1);
	#endif
}

bool Background() {
	#if defined unix
		pid_t pid = fork();
		if(pid == 0) {
			close(0); open("/dev/null", O_RDONLY);
			close(1); open("/dev/null", O_WRONLY);
			close(2); open("/dev/null", O_WRONLY);
			setsid();
			return true;
		} else {
			if(pid < 0) {
				std::cerr << "Error in Forking: " << strerror(errno);
			} else {
				std::cout << "New Process ID: " << pid;
			}
			PanicExit(); //It's not really panic...
			return false;
		}
	#elif defined WIN32
		#ifdef _MSC_VER
			#pragma warning(disable: 4800)
		#endif
		return FreeConsole();
	#endif
}
