//#include <stdio.h>
#include <stdlib.h>
#include <iostream>


//Sockets includes
#define WIN_LEAN_AND_MEAN
#define NOMINMAX
#include <WinSock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>

void bind()
{

}

void recvFrom()
{

}

void sendTo()
{

}

void closeSocket()
{

}


bool initializeSocketsLib()
{
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != NO_ERROR)
	{
		std::cout << "Server initialization: FAILED \n";
		std::cout << "----------\n";
		return false;
	}
	std::cout << "Server initialization: SUCCESS \n";
	std::cout << "----------\n";
	return true;
}

bool cleanUpSocketsLib()
{
	int iResult = WSACleanup();

	if (iResult == NO_ERROR)
	{
		//SUCCESS!
		std::cout << "----------\n";
		std::cout << "Sockets CleanUp: SUCCESS \n";
		return true;
	}

	//FAILURE!
	std::cout << "----------\n";
	std::cout << "Sockets CleanUp: FAILURE \n";
	return false;
}



//UDP CLIENT Entry Point
int main(int argc, char** argv)
{
	std::cout << "------------------------------------------------- UDP SERVER Start! ------------------------------------------------- \n";
	//Initialize the Sockets library
	initializeSocketsLib();

	//Create a UDP Socket!!! ----------------------------------------------------------------------------------------
	SOCKET serv_s = socket(AF_INET, SOCK_DGRAM, 0);

	//Create  & SETUP Bind Adress
	sockaddr_in bindAddr;
	bindAddr.sin_family = AF_INET; //IPv4

	short int port = 8000;		//Port we've set-up in Visual Studio
	bindAddr.sin_port = htons(port);
	bindAddr.sin_addr.S_un.S_addr = INADDR_ANY;
	//const char* remoteAddrStr = "127.0.0.1";
	//inet_pton(AF_INET, remoteAddrStr, &bindAddr);   //Convert string to proper in-library IP address

	//Bind the Adrress
	bind(serv_s, (const struct sockaddr*) &bindAddr, sizeof(bindAddr));

	//-----------------------------------------------------------------------------------------------------------------


	for (int i = 0; i < 5; ++i)
	{
		std::cout << "Cycle:" << i + 1<< " \n";
		Sleep(500);	//Wait 500 ms
	}


	//Close the Socket
	if (closesocket(serv_s) == NO_ERROR)
	{
		std::cout << "Closed Server Socket";
	}
	//CleanUP Sockets
	cleanUpSocketsLib();



	system("pause");
	return 1;
}