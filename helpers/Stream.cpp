
#include "Stream.h"

#include <stdarg.h>
#include <sstream>
#include "StringCollector.hpp"

#ifdef unix
	#include <errno.h>
#elif defined WIN32
	#define close CloseHandle
	//ReadFile and WriteFile have Differing arguments
#endif

StreamIn::StreamIn() : fd_in(NULL), rv(0), buff(buffaddr) {}

#ifdef unix
	StreamIn::StreamIn(int fd) : fd_in(fd), rv(0), buff(buffaddr) {}
#elif defined win32
	StreamIn::StreamIn(HANDLE fd) : fd_in(fd), rv(0), buff(buffaddr) {}
#endif

StreamIn::~StreamIn(){
	Close();
}

bool StreamIn::ReadLine(std::string * line){
	std::stringstream ss;
	do{
		if(buffaddr+rv > buff){ //I left things behind, last time.
			buffaddr2=buff;
			while(*buff && *buff != '\r' && *buff != '\n') buff++;
			if(*buff == '\r' || *buff == '\n'){ //This is the case, when there was a completed line found.
				*buff=0;
				buff++;
				if((buff - buffaddr < STREAM_MAXCHARS) && (*buff == '\r' || *buff == '\n') && *buff != *(buff-1)) buff++; //This will ignore a secondary newline character coming from windows (or a wierd \n\r newline) (But it won't ignore \n\n)
				ss << buffaddr2;
				break;
			}
			ss << buffaddr2;
		}
		buff = buffaddr;
		if(!ReadFinal())
		{
			#ifdef WIN32
				if(GetLastError() != ERROR_BROKEN_PIPE) throw "Something bad happend with a pipe";
			#elif defined unix
				Close();
			#endif
			try {
				*line = ss.str();
			} catch (...) {
				*line = "";
			}
			return false;
		}
		buff = buffaddr;
		*(buff+rv)=0;
	} while(*buff!=0);
	*line = ss.str();
	return true;
}

void StreamIn::Close() {
	try {
		if(fd_in) close(fd_in);
	} catch (...) {}
}

bool StreamIn::ReadFinal() {
	#ifdef unix
		do rv = read(fd_in, buff, STREAM_MAXCHARS-1);
		while(rv == -1 && errno == EINTR);
		return(rv > 0);
	#elif WIN32
		return(ReadFile(fd_in, buff, STREAM_MAXCHARS-1, &rv, NULL) || rv>0);
	#endif
}



StreamOut::StreamOut() : fd_out(NULL) {}

StreamOut::~StreamOut(){
	Close();
}

bool StreamOut::Write(const char * first, ...){
	if(!fd_out) return false;
	va_list vl;
	va_start(vl, first);
	WriteList(first, vl);
	va_end(vl);
	return true;
}

bool StreamOut::WriteList(const char * first, va_list & vl){
	if(!fd_out) return false;
	StringCollector msg;
	msg.Push(first, false);
	const char * str;
	while((str = va_arg(vl, const char *))) msg.Push(str, false);
	if(*(msg.GetBlock()+msg.GetLength()-1) != '\r' && *(msg.GetBlock()+msg.GetLength()-1) != '\n') msg.Push("\n");
	return WriteFinal(msg.GetBlock(), msg.GetLength());
}

bool StreamOut::WriteFinal(const char * msg, int len){
	#ifdef unix
		int rv;
		do rv=write(fd_out, msg, len);
		while(rv==-1 && errno != EINTR);
		return (rv==len);
	#elif WIN32
		DWORD br;
		return(WriteFile(fd_out, msg, len, &br, NULL) && br==len);
	#endif
}

void StreamOut::Close() {
	try {
		if(fd_out) close(fd_out);
	} catch (...) {}
}


void Stream::ClosePipes() {
	this->StreamIn::Close();
	this->StreamOut::Close();
}

void Stream::GetStandardIO(Stream & toset) {
	#ifdef unix
		toset.fd_out = STDOUT_FILENO;
		toset.fd_in  = STDIN_FILENO;
	#elif WIN32
		toset.fd_out = GetStdHandle(STD_OUTPUT_HANDLE);
		toset.fd_in  = GetStdHandle(STD_INPUT_HANDLE);		
	#endif
}