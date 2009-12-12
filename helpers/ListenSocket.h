#ifndef ListenSocketH
#define ListenSocketH

#include <string>

#ifdef UNIX
	#include "Stream.h"
#elif defined WIN32
	#include <winsock2.h>
#endif

#ifndef STREAM_MAXCHARS
	#define STREAM_MAXCHARS 128
#endif

class ListenSocket;

class Connection : public Stream { //Under unix, this implements nothing new. 
	friend ListenSocket;
	#ifdef WIN32
		private:
			SOCKET client;
		public:
			~Connection();
			bool ReadLine(std::string *, const char [2]);
			bool Write(const char *, ...);
			bool Write(const char *, va_list);
			void Close();
	#endif
};

class ListenSocket {
	private:
		#ifdef UNIX
			int       list_s;                /*  listening socket          */
			int       conn_s;                /*  connection socket         */
			struct    sockaddr_in servaddr;  /*  socket address structure  */
		#elif defined WIN32
    		sockaddr_in from;
			SOCKET server;
		#endif
	public:
		ListenSocket();
		~ListenSocket();
		bool Init(int port);
		bool AwaitConnection(Connection *);
};

#endif