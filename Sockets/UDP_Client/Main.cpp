//#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <Windows.h>


//UDP CLIENT Entry Point
int main(int argc, char **argv)
{
	std::cout << "UDP Client Start! \n";

	for (int i = 0; i < 5; ++i)
	{
		std::cout << "Cycle:" <<  i << " \n";
		Sleep(500);
	}

	system("pause");
	return 1;
}