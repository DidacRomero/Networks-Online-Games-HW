//#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <Windows.h>

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

//UDP CLIENT Entry Point
int main(int argc, char** argv)
{
	std::cout << "UDP Server Start! \n";

	for (int i = 0; i < 5; ++i)
	{
		std::cout << "Cycle:" << i + 1<< " \n";
		Sleep(500);	//Wait 500 ms
	}

	system("pause");
	return 1;
}