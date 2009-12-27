
#include "ListenSocket.h"

#include "StringCollector.hpp"
#include <sstream>
#if defined WIN32
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

	bool Connection::ReadFinal() {
		return ((rv=recv(client, buff, STREAM_MAXCHARS-1, 0)) != SOCKET_ERROR || rv!=0);
	}

	void Connection::Close() {
		if(client) closesocket(client);
	}

	bool Connection::WriteFinal(const char * msg, int length) {
		return(send(client, msg, length, 0) != SOCKET_ERROR);
	}
#endif