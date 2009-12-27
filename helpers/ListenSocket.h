#ifndef ListenSocketH
#define ListenSocketH

#ifdef unix
	#include <sys/socket.h>
	#include <arpa/inet.h>
#elif defined WIN32
	#include <winsock2.h> //important to include that before windows.h, which is included by Stream.h
#endif
#include <string>
#include "Stream.h"

#ifndef STREAM_MAXCHARS
	#define STREAM_MAXCHARS 128
#endif

class ListenSocket;

class Connection : public Stream { //Under unix, this implements nothing new. 
	friend class ListenSocket;
	#ifdef WIN32
		private:
			SOCKET client;
			bool WriteFinal(const char *, int);
		public:
			~Connection();
			bool Connection::ReadFinal();
			bool Write(const char *, ...);
			bool WriteList(const char *, va_list &);
			void Close();
	#endif
};

class ListenSocket {
	private:
		#ifdef unix
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