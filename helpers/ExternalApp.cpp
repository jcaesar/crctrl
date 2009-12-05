
#include "ExternalApp.h"

#include "StringFunctions.h"
#include "StringCollector.hpp"

#if defined UNIX
	#include <errno.h>
	#include <signal.h>
	#include <sys/wait.h>
#elif defined WIN32
	#include <windows.h>
	#pragma comment(lib, "Ws2_32.lib")
#endif

Process::Process() : path(NULL), args(NULL), Status(PreRun) {
	#ifdef UNIX
		pid = 0;
	#elif defined WIN32
		hChildProcess = NULL;
	#endif
}

Process::~Process() {
	Kill(true);
	delete [] args;
	delete [] path;
}

bool Process::SetArguments(const char * spath, const char * sargs) {
	if(Status != PreRun || (!spath && !sargs)) return false;
	if(spath) {
		if(path) delete [] path;
		path = new char [strlen(spath)+1];
		strcpy(path, spath);
	}
	if(sargs) {
		if(args) delete [] args;
		args = new char [strlen(sargs)+1];
		strcpy(args, sargs);
		#ifdef WIN32
			strcharreplace(args, '/', '\\');
		#endif
	}
	return true;
}

bool Process::Start() {
	if(Status != PreRun) return false;
	#ifdef UNIX
		int fd1[2];
		int fd2[2];
		//int fderr[2];
		int fdtmp[2];
		pid_t child;
		worker = pthread_self();
		Status=PreLobby;
		if ( (pipe(fd1) < 0) || (pipe(fd2) < 0) /*|| (pipe(fderr) < 0)*/ || (pipe(fdtmp) < 0) ){
			std::cerr << "Error opening Pipes." << std::endl;
			return false;
		}
		child = fork();
		if(child < 0) { std::cerr << "Error forking process." << std::endl; return false;}
		if(child == 0){
			close(fdtmp[0]);
			child = fork();
			if(child < 0) exit(1);
			if(child == 0){
				close(fd1[1]);
				close(fd2[0]);
				//close(fderr[0]);
				if (fd1[0] != STDIN_FILENO){
					if (dup2(fd1[0], STDIN_FILENO) != STDIN_FILENO) std::cout << "Error with stdin" << std::endl;
					close(fd1[0]);
				}
				if (fd2[1] != STDOUT_FILENO){
					if (dup2(fd2[1], STDOUT_FILENO) != STDOUT_FILENO) std::cout << "Error with stdout" << std::endl;
					close(fd2[1]);
				}
				/*if (fderr[1] != STDERR_FILENO){
					if (dup2(fderr[1], STDERR_FILENO) != STDERR_FILENO) std::cout << "Error with stderr" << std::endl;
					close(fderr[1]);
				}*/

				execl(path, args, NULL);
				//When execl does fine, it will never return.
				std::cout << "Call to execl failed. Error: " << strerror(errno) << std::endl; //Give parent process a notice.
				close(fd1[0]);
				close(fd2[1]);
				//close(fderr[1]);
			}
			char pidchr [12];
			sprintf(pidchr, "%d\n", child);
			write(fdtmp[1], pidchr, strlen(pidchr));
			close(fdtmp[1]);
			raise(SIGKILL);
		}
		close(fdtmp[1]);
		waitpid(child, NULL, 0);
		StreamReader pidread (fdtmp[0]);
		std::string pids;
		if(pidread.ReadLine(&pids)) pid = atoi(pids.c_str());
		else pid = 0;
		close(fdtmp[0]);
		close(fd1[0]);
		close(fd2[1]);
		//close(fderr[1]);
		fd_in  = fd2[0];
		fd_out = fd1[1];
		//if(pipe_err) close(pipe_err);
		//pipe_err=fderr[0];
	#elif defined WIN32
		SECURITY_ATTRIBUTES sa;
		sa.nLength= sizeof(SECURITY_ATTRIBUTES);
		sa.lpSecurityDescriptor = NULL;
		sa.bInheritHandle = TRUE;
		HANDLE hInputWriteTmp, hInputRead;
		HANDLE hOutputReadTmp, hOutputWrite;
		HANDLE hErrorWrite;
		if (!CreatePipe(&hOutputReadTmp,&hOutputWrite,&sa,0))
			return false;
		if (!DuplicateHandle(GetCurrentProcess(),hOutputWrite, GetCurrentProcess(),&hErrorWrite,0,TRUE,DUPLICATE_SAME_ACCESS))
			return false;
		if (!CreatePipe(&hInputRead,&hInputWriteTmp,&sa,0))
			return false;
		if (!DuplicateHandle(GetCurrentProcess(),hOutputReadTmp,
									GetCurrentProcess(),
									&fd_out, // Address of new handle.
									0,FALSE, // Make it uninheritable.
									DUPLICATE_SAME_ACCESS))
			return false;

		if (!DuplicateHandle(GetCurrentProcess(),hInputWriteTmp,
									GetCurrentProcess(),
									&fd_in, // Address of new handle.
									0,FALSE, // Make it uninheritable.
									DUPLICATE_SAME_ACCESS))
			return false;
		if (!CloseHandle(hOutputReadTmp)) return false;
		if (!CloseHandle(hInputWriteTmp)) return false;
		PROCESS_INFORMATION pi;
		STARTUPINFO si;
		ZeroMemory(&si,sizeof(STARTUPINFO));
		si.cb = sizeof(STARTUPINFO);
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdOutput = hOutputWrite;
		si.hStdInput  = hInputRead;
		si.hStdError  = hErrorWrite;
		// Use this if you want to hide the child:
		si.wShowWindow = SW_HIDE;
		int lenp = strlen(path); int lena = strlen(args);
		char * cmd = new char [lenp + lena + 3];
		*cmd = '"';
		strcpy(cmd+1,path);
		*(cmd+lenp) = '"';
		strcpy(cmd+1+lenp,path);
		*(cmd+lenp+lena+3) = NULL;
		if (!CreateProcess(NULL,cmd,NULL,NULL,TRUE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi))
			return false;
		hChildProcess = pi.hProcess;
		if (!CloseHandle(pi.hThread)) return false;
		if (!CloseHandle(hOutputWrite)) return false;
		if (!CloseHandle(hInputRead)) return false;
		if (!CloseHandle(hErrorWrite)) return false;
	#endif
	return true;
}

bool Process::Kill(bool brute) {
	#ifdef UNIX
		return !kill(pid, brute?SIGKILL:SIGTERM);
	#elif defined WIN32
		#pragma warning (disable: 4800)
		return TerminateProcess(hChildProcess, -1);
	#endif	
}

bool Process::Wait(bool noblock) {
	#ifdef UNIX
		int status;
		waitpid(pid, &status, noblock?WNOHANG);
		return status;
	#elif defined WIN32
		return WaitForSingleObject(hChildProcess, noblock?1:INFINITE)!=WAIT_FAILED;
	#endif		
}

void Process::ClosePipeTo() {
	this->StreamOut::Close();
}