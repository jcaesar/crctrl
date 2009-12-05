
#include "Stream.h"

#include <stdarg.h>
#include <sstream>
#include "StringCollector.hpp"

#ifdef WIN32
	#define close CloseHandle
	//ReadFile and WriteFile have Differing arguments
#endif

StreamIn::StreamIn() : buff(buffadr), rv(0), fd_in(NULL) {}

StreamIn::~StreamIn(){
	Close();
}

bool StreamIn::ReadLine(std::string * line, const char delim [2]){
	std::stringstream ss;
	do{
		if(buffadr+rv > buff){ //I left things behind, last time.
			buffadr2=buff;
			while(*buff && !strcmp(buff,delim)) buff++;
			if(strcmp(buff,delim)){ //This is the case, when there was a completed line found.
				*buff=0;
				buff++;
				ss << buffadr2;
				break;
			}
			ss << buffadr2;
		}
		buff = buffadr;
		#ifdef UNIX
		if((rv = read(fd_in, buff, STREAM_MAXCHARS-1)) <= 0) 
		#elif WIN32
		if(!ReadFile(fd_in, buff, STREAM_MAXCHARS-1, &rv, NULL) || rv==0)
		#endif
		{
			Close();
			*line = ss.str();
			return false;
		}
		buff = buffadr;
		*(buff+rv)=0;
	} while(*buff!=0);
	*line = ss.str();
	return true;
}

void StreamIn::Close() {
	if(fd_in) close(fd_in);
}


StreamOut::StreamOut() : fd_out(NULL) {}

StreamOut::~StreamOut(){
	Close();
}

bool StreamOut::Write(const char * first, ...){
	if(!fd_out) return false;
	va_list vl;
	va_start(vl, first);
	Write(first, vl);
	va_end(vl);
	return true;
}

bool StreamOut::Write(const char * first, va_list vl){
	if(!fd_out) return false;
	StringCollector msg;
	const char * str;
	while((str = va_arg(vl, const char *))) msg.Push(str);
	msg.GetBlock();
	#ifdef UNIX
		return (write(fd_out, msg.GetBlock(), msg.GetLength())==msg.GetLength());
	#elif WIN32
		DWORD br;
		return(WriteFile(fd_out, msg.GetBlock(), msg.GetLength(), &br, NULL) && br==msg.GetLength());
	#endif
}

void StreamOut::Close() {
	if(fd_out) close(fd_out);
}


void Stream::ClosePipes() {
	this->StreamIn::Close();
	this->StreamOut::Close();
}

void Stream::GetStandardIO(Stream & toset) {
	#ifdef UNIX
		toset.fd_out = STDOUT_FILENO;
		toset.fd_in  = STDIN_FILENO;
	#elif WIN32
		toset.fd_out = GetStdHandle(STD_OUTPUT_HANDLE);
		toset.fd_in  = GetStdHandle(STD_INPUT_HANDLE);		
	#endif
}