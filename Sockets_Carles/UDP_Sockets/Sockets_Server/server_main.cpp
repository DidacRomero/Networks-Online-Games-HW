#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>

/* ----------------------------------------

SOCKET socket(int af, int type, int protocol) // Create socket
	af = AF_INET (IPv4) or AF_INET6 (IPv6)
	type = SOCK_DGRAM (UDP) or SOCK_STREAM (TCP)

int shutdown(SOCKET s, int direction) // Stop socket data transfer
	direction = SD_RECIEVE or SD_SEND or SD_BOTH (transfer direction to stop)

int closesocket(SOCKET s)	// Destroy socket

struct sockaddr_in {
	short sin_family;			// Address family (IPv4/6)
	unsigned short sin_port;	// Port
	struct in_addr sin_addr;	// IP Address
	char sin_zero [8];			// Not used
};

int bind(SOCKET s, const struct sockaddr* address, int address_len);	// Bind socket to an IP address

------------------------------------------- */

int main(int argc, char* argv[])
{
	// Initialization
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult == NO_ERROR) {
		// UDP
		SOCKET s = socket(AF_INET, SOCK_DGRAM, 0);	// Create socket

		if (s != INVALID_SOCKET && s != SOCKET_ERROR) {

			// Save local address
			const char* remoteAddrStr = "127.0.0.1";	// Localhost / Local Adress

			// Setting up the socket to recieve connections and data
			struct sockaddr_in bindAddr;
			bindAddr.sin_family = AF_INET;				// IPv4
			bindAddr.sin_port = htons(8000);			// Port
			bindAddr.sin_addr.S_un.S_addr = INADDR_ANY;	// Any local IP address

			// Failsave to ensure that binding doesn't return SOCKET_ERROR if the used socket hasn't been properly closed before
			//int enable = 1;
			//int res = setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));
			int res = NULL;

			if (res != SOCKET_ERROR) {

				// Bind server socket to a IP address
				res = bind(s, (const struct sockaddr*)&bindAddr, sizeof(bindAddr));

				int adressSize = sizeof(sockaddr_in);
				const unsigned int bufferSize = 256;
				char* msg = new char[bufferSize];
				const char* response = "Pong!";
				bool shutdownServer = false;

				int cycle = 0;

				//Sleep(1000);
				while (!shutdownServer) {
					
					sockaddr_in remoteAddr;

					printf("SERVER ITERATION: %i\n", ++cycle);
					if (recvfrom(s, msg, bufferSize, 0, (struct sockaddr*)&remoteAddr, &adressSize) != SOCKET_ERROR) {

						printf("Server Receives: %s\n", msg);

						if (strcmp(msg, "TERMINATE\n") == 0) {
							shutdownServer = true;
						}
						else {
							if (sendto(s, response, bufferSize, 0, (const struct sockaddr*)&remoteAddr, sizeof(remoteAddr)) != SOCKET_ERROR) {
								printf("Server Sends: %s\n", response);
							}
							else {
								iResult = WSAGetLastError();
								printf("Message Sending Error! Error code: %i\n", iResult);
								break;
							}
						}
					}
					else {
						iResult = WSAGetLastError();
						printf("Message Receving Error! Error code: %i\n", iResult);
						break;
					}
					
					strcpy_s(msg, bufferSize, "");
					printf("\n");
				}

				delete[] msg;
			}
			else {
				iResult = WSAGetLastError();
				printf("Unknown Error! Error code: %i\n", iResult);
			}
		}
		else {
			iResult = WSAGetLastError();
			printf("Socket Creation Error! Error code: %i\n", iResult);
		}

		closesocket(s);	// Destroy socket
		iResult = WSACleanup();	
	}
	else {
		iResult = WSAGetLastError();
		printf("WSAStartup Error! Error code: %i\n", iResult);
	}

	system("pause");
	return 0;
}