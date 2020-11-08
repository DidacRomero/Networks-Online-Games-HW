#include "Networks.h"
#include "ModuleNetworking.h"
#include <list>
#include <iterator>


static uint8 NumModulesUsingWinsock = 0;

//TODO 0: (JESUS)
bool ModuleNetworking::sendPacket(const OutputMemoryStream & packet, SOCKET socket)
{
	int result = send(socket, packet.GetBufferPtr(), packet.GetSize(), 0);
	if (result == SOCKET_ERROR)
	{
		reportError("send");
		return false;
	}
	return true;
}


void ModuleNetworking::reportError(const char* inOperationDesc)
{
	LPVOID lpMsgBuf;
	DWORD errorNum = WSAGetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	ELOG("Error %s: %d- %s", inOperationDesc, errorNum, lpMsgBuf);
}

void ModuleNetworking::disconnect()
{
	for (SOCKET socket : sockets)
	{
		shutdown(socket, 2);
		closesocket(socket);
	}

	sockets.clear();
}

bool ModuleNetworking::init()
{
	if (NumModulesUsingWinsock == 0)
	{
		NumModulesUsingWinsock++;

		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		if (WSAStartup(version, &data) != 0)
		{
			reportError("ModuleNetworking::init() - WSAStartup");
			return false;
		}
	}

	return true;
}

bool ModuleNetworking::preUpdate()
{
	if (sockets.empty()) return true;

	// NOTE(jesus): TODO:0 (Jesus) now we will use packet instead of bytes
	InputMemoryStream packet;

	// TODO(jesus): select those sockets that have a read operation available
	//This code is extracted from Jesus's pdf
	fd_set readfds;
	FD_ZERO(&readfds);

	//Fill the set
	for (auto s : sockets)
		FD_SET(s, &readfds);

	//Timeout (0 to return immediately)
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	//Select (Check for Readibility)
	int res = select(0, &readfds, nullptr, nullptr, &timeout);
	if (res == SOCKET_ERROR)
	{
		LOG("Error %s", WSAGetLastError());
		return true;
	}

	// TODO(jesus): for those sockets selected, check wheter or not they are
	// a listen socket or a standard socket and perform the corresponding
	// operation (accept() an incoming connection or recv() incoming data,
	// respectively).

	//------------------------Checking selected sockets ----------------------------------//
	std::list<SOCKET> disconnectedSockets;
	//Read selscted sockets
	for (auto s : sockets)
	{
		if (FD_ISSET(s, &readfds))
		{
			if (isListenSocket(s))		//If we are the server socket
			{	//ACCEPT stuff
				sockaddr_in received_addr;
				int addr_size = sizeof(received_addr);
				// On accept() success, communicate the new connected socket to the
				// subclass (use the callback onSocketConnected()), and add the new
				// connected socket to the managed list of sockets.
				SOCKET remote_s = accept(s, (struct sockaddr*)&received_addr, &addr_size);
				if (remote_s != -1) //Success
				{
					onSocketConnected(remote_s, received_addr);
					addSocket(remote_s);
					DLOG("Accepted a  remote socket");
				}
				else
				{
					DLOG("Error trying to accept a remote socket");
				}
			}
			else //It's a CLIENT, so we recv stuff
			{
				// On recv() success, communicate the incoming data received to the
				// subclass (use the callback onSocketReceivedData()).
				//TODO: 0 (JESUS) adapt receiving end to packet
				int bytesRead = recv(s, packet.GetBufferPtr(), packet.GetCapacity(), 0);
				//int result = recv(s, (char*) incomingDataBuffer, incomingDataBufferSize, 0);
				if (bytesRead > 0)
				{	//Success receiving PACKET data!
					packet.SetSize((uint32)bytesRead);
					onSocketReceivedData(s, packet);
				}
				else if (bytesRead == SOCKET_ERROR || bytesRead == 0 || bytesRead == ECONNRESET)	//Connection Terminated 
				{
					// TODO(jesus): handle disconnections. Remember that a socket has been
					// disconnected from its remote end either when recv() returned 0,
					// or when it generated some errors such as ECONNRESET.
					// Communicate detected disconnections to the subclass using the callback
					// onSocketDisconnected().
					LOG("Socket %d disconnected! Connection Terminated", s);
					onSocketDisconnected(s);

					disconnectedSockets.push_back(s);
				}
				else
				{
					//DLOG("Received: &s", incomingDataBuffer);
				}
			}

			
		}
	}

	// TODO(jesus): Finally, remove all disconnected sockets from the list
	// of managed sockets.
	if (disconnectedSockets.size() > 0)
	{
		//Double for to iterate disconnected sockets
		for (std::list<SOCKET>::iterator disc_it = disconnectedSockets.begin(); disc_it != disconnectedSockets.end(); ++disc_it)
		{
			for (std::vector<SOCKET>::iterator it = sockets.begin(); it != sockets.end(); it++)
			{
				if ((*disc_it) == (*it))
				{
					sockets.erase(it);
					break;
				}
			}
		}
	}

	return true;
}

bool ModuleNetworking::cleanUp()
{
	disconnect();

	NumModulesUsingWinsock--;
	if (NumModulesUsingWinsock == 0)
	{

		if (WSACleanup() != 0)
		{
			reportError("ModuleNetworking::cleanUp() - WSACleanup");
			return false;
		}
	}

	return true;
}

void ModuleNetworking::addSocket(SOCKET socket)
{
	sockets.push_back(socket);
}
