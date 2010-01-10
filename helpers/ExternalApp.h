#ifndef ExternalAppH
#define ExternalAppH

#include "Stream.h"

class Process : public Stream {
	private:
		enum { PreRun, Active, Stopped } Status;
		char * path;
		char * args;
		char * dir;
		#if defined unix
			pid_t pid;
		#elif defined WIN32
			HANDLE hChildProcess;
		#endif
	public:
		Process();
		~Process();
		bool SetArguments(const char * path, const char * args, const char * dir);
		bool Start();
		bool Kill(bool brutal = false);
		bool Wait(bool block = true);
		void ClosePipeTo();
		bool IsRunning() {return Status==Active;}
};

#endif