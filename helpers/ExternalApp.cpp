
#include "ExternalApp.h"

#include "StringFunctions.h"
#include "StringCollector.hpp"

#if defined unix
	#include <errno.h>
	#include <signal.h>
	#include <sys/wait.h>
	#include <cstdlib>
#elif defined WIN32
	#include <windows.h>
	#pragma comment(lib, "Ws2_32.lib")
#endif

Process::Process() : Status(PreRun), path(NULL), args(NULL), dir(NULL) {
	#ifdef unix
		pid = 0;
	#elif defined WIN32
		hChildProcess = NULL;
	#endif
}

Process::~Process() {
	Kill(true);
	delete [] args;
	delete [] path;
	delete [] dir;
}

bool Process::SetArguments(const char * spath, const char * sargs, const char * sdir) {
	if(Status != PreRun || (!spath && !sargs)) return false;
	if(spath) {
		delete [] path;
		path = new char [strlen(spath)+1];
		strcpy(path, spath);
		#ifdef WIN32
			strcharreplace(path, '/', '\\');
		#endif
	}
	if(sargs) {
		delete [] args;
		args = new char [strlen(sargs)+1];
		strcpy(args, sargs);
	}
	if(sdir) {
		delete [] dir;
		dir = new char [strlen(sdir)+1];
		strcpy(dir, sdir);
	}
	return true;
}

bool Process::Start() {
	if(Status != PreRun) return false;
	#ifdef unix
		int fd1[2];
		int fd2[2];
		//int fderr[2];
		int fdtmp[2];
		pid_t child;
		if ( (pipe(fd1) < 0) || (pipe(fd2) < 0) /*|| (pipe(fderr) < 0)*/ || (pipe(fdtmp) < 0) ) return false;
		child = fork();
		if(child < 0) throw "Could not fork.";
		if(child == 0){
			close(fdtmp[0]);
			child = fork();
			if(child < 0) exit(1);
			if(child == 0){
				close(fd1[1]);
				close(fd2[0]);
				//close(fderr[0]);
				if (fd1[0] != STDIN_FILENO){
					if (dup2(fd1[0], STDIN_FILENO) != STDIN_FILENO) close(STDIN_FILENO);
					close(fd1[0]);
				}
				if (fd2[1] != STDOUT_FILENO){
					if (dup2(fd2[1], STDOUT_FILENO) != STDOUT_FILENO) close(STDOUT_FILENO); //FIXME: close() is a very bad error handling
					close(fd2[1]);
				}
				/*if (fderr[1] != STDERR_FILENO){
					//if (dup2(fderr[1], STDERR_FILENO) != STDERR_FILENO) err
					close(fderr[1]);
				}*/
				close(STDERR_FILENO);
				if(dir) chdir(dir);
				execl(path, "", args, NULL);
				//When execl does fine, it will never return.
				close(fd1[0]);
				close(fd2[1]);
				//close(fderr[1]);
				raise(SIGKILL);
			}
			char pidchr [12];
			sprintf(pidchr, "%d\n", child);
			write(fdtmp[1], pidchr, strlen(pidchr));
			close(fdtmp[1]);
			raise(SIGKILL);
		}
		close(fdtmp[1]);
		waitpid(child, NULL, 0);
		StreamIn pidread (fdtmp[0]);
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
		//HANDLE hErrorWrite;
		if (!CreatePipe(&hOutputReadTmp,&hOutputWrite,&sa,0))
			return false;
		//if (!DuplicateHandle(GetCurrentProcess(),hOutputWrite, GetCurrentProcess(),&hErrorWrite,0,TRUE,DUPLICATE_SAME_ACCESS))
		//	return false;
		if (!CreatePipe(&hInputRead,&hInputWriteTmp,&sa,0))
			return false;
		if (!DuplicateHandle(GetCurrentProcess(),hOutputReadTmp, GetCurrentProcess(), &fd_in,0,FALSE,DUPLICATE_SAME_ACCESS))
			return false;
		if (!DuplicateHandle(GetCurrentProcess(),hInputWriteTmp, GetCurrentProcess(), &fd_out ,0,FALSE,DUPLICATE_SAME_ACCESS))
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
		//si.hStdError  = hErrorWrite;
		si.wShowWindow = SW_HIDE;
		int lenp = strlen(path); int lena = strlen(args);
		char * cmd = new char [lenp + lena + 4];
		*cmd = '"';
		strcpy(cmd+1,path);
		strcpy(cmd+lenp+1, "\" ");
		strcpy(cmd+3+lenp,args);
		if (!CreateProcess(NULL,cmd,NULL,NULL,TRUE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi))
			return false;
		hChildProcess = pi.hProcess;
		if (!CloseHandle(pi.hThread)) return false;
		if (!CloseHandle(hOutputWrite)) return false;
		if (!CloseHandle(hInputRead)) return false;
		//if (!CloseHandle(hErrorWrite)) return false;
	#endif
	Status = Active;
	return true;
}

bool Process::Kill(bool brute) {
	#ifdef unix
		if(pid>0) return !kill(pid, brute?SIGKILL:SIGTERM);
		else return 0;
	#elif defined WIN32
		#pragma warning (disable: 4800)
		return TerminateProcess(hChildProcess, -1);
	#endif	
}

bool Process::Wait(bool noblock) {
	#ifdef unix
		int status;
		waitpid(pid, &status, noblock?WNOHANG:0);
		return status;
	#elif defined WIN32
		return WaitForSingleObject(hChildProcess, noblock?1:INFINITE)!=WAIT_FAILED;
	#endif		
}

void Process::ClosePipeTo() {
	this->StreamOut::Close();
}