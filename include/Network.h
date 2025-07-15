#pragma once
typedef unsigned short USHORT;
typedef void* SocketHandle;

class Network
{
public:
	bool init();
	bool shutdown();

	//@TODO agregar otros protocolos
	SocketHandle createSocket();
	bool setNonBlocking(SocketHandle socket);
	bool bindServer(SocketHandle socket, USHORT port);
	bool sendTo(SocketHandle socket, const char* msg, const char* ip, USHORT port);
	bool receiveFrom(SocketHandle socket, char* buffer, int bufferSize);
	//int receive(SocketHandle socket, char* buffer, int bufferSize);
	//int send(SocketHandle socket, const char* buffer, int bufferSize);

};

