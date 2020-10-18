//#include <stdio.h>
#include <stdlib.h>
#include <iostream>

//Sockets includes
#define WIN_LEAN_AND_MEAN
#define NOMINMAX
#include <WinSock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>

void sendTo()
{

}

void recvFrom()
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
		//Initialization FAILURE
		std::cout << "Server initialization: FAILED \n";
		std::cout << "----------\n";
		return false;
	}

	// Initialization SUCCESS
	std::cout << "Server initialization: SUCCESS \n";
	std::cout << "----------\n";
	return true;
}

//UDP CLIENT Entry Point
int main(int argc, char **argv)
{
	std::cout << "------------------------------------------------- UDP CLIENT Start! ----------------------------------------------------\n";
	//Initialize Sockets
	initializeSocketsLib();

	for (int i = 0; i < 5; ++i)
	{
		std::cout << "Cycle:" <<  i + 1 << " \n";
		Sleep(500);	//Wait 500 ms
	}

	system("pause");
	return 1;
}