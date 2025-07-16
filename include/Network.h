#pragma once
/*
	Envoltura de la API WinSock2
	Eventualmente podría envolver la api sockets de gnu/linux
*/
typedef unsigned short USHORT;
typedef void* SocketHandle;
typedef void* AddressHandle;

class Network
{
public:
	bool init();
	bool shutdown();

	//@TODO agregar otros protocolos
	SocketHandle createSocket();
	bool setNonBlocking(SocketHandle socket);
	bool bindServer(SocketHandle socket, USHORT port);
	//para enviar de cliente a servidor 
	bool sendTo(SocketHandle socket, const char* msg, const char* ip, USHORT port);
	//para enviar de servidor a cliente conectado
	bool sendTo(SocketHandle socket, const char* msg, const AddressHandle address);
	bool receiveFrom(SocketHandle socket, char* buffer, int bufferSize, AddressHandle remoteAddress);
	//retorna el tamano de sockaddr_in
	size_t sizeOfsockaddr_in();

};

