
#include "ListenSocket.h"

#include "StringCollector.hpp"
#include <sstream>
#if defined WIN32
	#include <winsock2.h>
	#include <conio.h>
	#include <iostream>
#endif

ListenSocket::ListenSocket() {
	#ifdef WIN32
		server = NULL;
	#endif
}

bool ListenSocket::Init(int port) {
	#ifdef unix
	    if ((list_s = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
			return false;
	    }
		memset(&servaddr, 0, sizeof(servaddr));
	    servaddr.sin_family      = AF_INET;
	    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	    servaddr.sin_port        = htons(port);
	    if (bind(list_s, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0) {
			return false;
	    }
	    if (listen(list_s, 8) < 0 ) {
			return false;
	    }
	#elif defined WIN32
		WSADATA wsaData;
		sockaddr_in local;
		int wsaret=WSAStartup(0x101,&wsaData);
		if(wsaret!=0)
			return false;
		local.sin_family=AF_INET; //Address family
		local.sin_addr.s_addr=INADDR_ANY; //Wild card IP address
		local.sin_port=htons((u_short)port); //port to use
		server=socket(AF_INET,SOCK_STREAM,0);
		if(server==INVALID_SOCKET)
		    return false;
		if(bind(server,(sockaddr*)&local,sizeof(local))!=0)
			return false;
		if(listen(server,10)!=0)
			return false;
	#endif
		return true;
}

bool ListenSocket::AwaitConnection(Connection * Conn) {
	#ifdef unix
		if ((conn_s = accept(list_s, NULL, NULL) ) < 0 ) {
			return false;
		}
		Conn->StreamIn::fd_in = conn_s;
		Conn->StreamOut::fd_out = conn_s;
	#elif defined WIN32
		int fromlen=sizeof(from);
		Conn->client=accept(server, (struct sockaddr*)&from, &fromlen);
		if(Conn->client == INVALID_SOCKET) return false;
	#endif
	return true;
}

ListenSocket::~ListenSocket() {
	#ifdef WIN32 //FIXME: Implement for Unix likewise
		closesocket(server);
	#endif 
}

#ifdef WIN32
	Connection::~Connection() {
		Close();
	}

	bool Connection::ReadLine(std::string * line, const char delim [3]){
		if(!client) return false;
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
			if((rv=recv(client, buff, STREAM_MAXCHARS-1, 0)) == SOCKET_ERROR || rv==0)
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

	void Connection::Close() {
		if(client) closesocket(client);
	}

	bool Connection::Write(const char * first, ...) {
		va_list vl;
		va_start(vl, first);
		WriteList(first, vl);
		va_end(vl);
		return true;
	}

	bool Connection::WriteList(const char * first, va_list vl) {
		StringCollector msg;
		const char * str;
		while((str = va_arg(vl, const char *))) msg.Push(str);
		msg.GetBlock();
		return(send(client, msg.GetBlock(), msg.GetLength(), 0) != SOCKET_ERROR);
	}
#endif