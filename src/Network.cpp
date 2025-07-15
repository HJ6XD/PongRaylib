#include "Network.h"
#include <WinSock2.h>
#pragma comment(lib, "Ws2_32.lib")



bool Network::init()
{
	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		return false;
	}
	return true;
}

SocketHandle Network::createSocket()
{
	// socket UDP 
	SOCKET s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (s == INVALID_SOCKET) 
	{
		return nullptr; // Error creating socket
	}
	return reinterpret_cast<SocketHandle>(s);
}

bool Network::setNonBlocking(SocketHandle socket)
{
	SOCKET s = reinterpret_cast<SOCKET>(socket);
	u_long mode = 1; 
	int result = ioctlsocket(s, FIONBIO, &mode);
	if (result == SOCKET_ERROR) {
		return false; // Error setting non-blocking mode
	}
	return true;
}

bool Network::bindServer(SocketHandle socket, USHORT port)
{
	sockaddr_in serverAddr = { 0 };
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	//servidor usa cualquier IP disponible
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	SOCKET s = reinterpret_cast<SOCKET>(socket);
	//conversion tipo C
	//sockaddr* address = (struct sockaddr*)&serverAddr;
	// conversion tipo C++
	sockaddr* address = reinterpret_cast<sockaddr*>(&serverAddr);
	int result = bind(s,address, sizeof(serverAddr));
	if (result == SOCKET_ERROR) {
		return false; // Error binding socket
	}
	return true;
}

//en UDP no hay conexión, se envían y reciben mensajes sin establecer una conexión previa
bool Network::sendTo(SocketHandle socket, const char* msg, const char* ip, USHORT port)
{
	sockaddr_in serverAddr = { 0 };	
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(ip);
	SOCKET s = reinterpret_cast<SOCKET>(socket);
	int msglen = static_cast<int>(strlen(msg));
	sockaddr* addr = reinterpret_cast<sockaddr*>(&serverAddr);
	int result = sendto(s, msg, msglen, 0, addr, sizeof(serverAddr));
	if (result == SOCKET_ERROR) {
		return false; 
	}
}

bool Network::sendTo(SocketHandle socket, const char* msg, const AddressHandle address)
{
	sockaddr_in* addrIn = reinterpret_cast<sockaddr_in*>(address);
	SOCKET s = reinterpret_cast<SOCKET>(socket);
	int msglen = static_cast<int>(strlen(msg));
	sockaddr* addr = reinterpret_cast<sockaddr*>(addrIn);
	int result = sendto(s, msg, msglen, 0, addr, sizeof(*addrIn));
	if (result == SOCKET_ERROR) {
		return false; 
	}
}

bool Network::receiveFrom(SocketHandle socket, char* buffer, int bufferSize, AddressHandle remoteAddress)
{
	sockaddr_in clientAddr = { 0 };
	int addrlen = sizeof(clientAddr);
	SOCKET s = reinterpret_cast<SOCKET>(socket);
	int result = recvfrom(s, buffer, bufferSize, 0, reinterpret_cast<sockaddr*>(&clientAddr), &addrlen);
	if (result > 0) {
		buffer[result] = '\0'; //seguro 
		//hacer copia de clientAddr
		if(remoteAddress!=nullptr)
			memcpy_s(remoteAddress, sizeof(sockaddr_in), &clientAddr, sizeof(sockaddr_in));
		return true; 
	}
	else {
		return false;
	}
}

size_t Network::sizeOfsockaddr_in()
{
	return sizeof(sockaddr_in);
}



bool Network::shutdown()
{
	int result = WSACleanup();
	if (result != 0) {
		return false;
	}
	return true; 
}
