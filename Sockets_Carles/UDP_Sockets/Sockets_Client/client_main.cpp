#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iostream>

// By Carles Homs & Dídac Romero

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

int sendto(SOCKET s, const char* buf, int len, int flags, const struct sockaddr* to, int tolen);	// Send datagram

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
			const char* remoteAddrStr = "127.0.0.1";	// Localhost / Local Adress

			struct sockaddr_in remoteAddr;
			remoteAddr.sin_family = AF_INET;			// IPv4
			remoteAddr.sin_port = htons(8000);			// Port

			if (inet_pton(AF_INET, remoteAddrStr, &remoteAddr.sin_addr) == 1) {
				
				int adressSize = sizeof(sockaddr_in);

				// Prepare message
				const char* msg = "Ping!";
				const unsigned int bufferSize = 256;
				char* response = new char[bufferSize];

				for (int i = 1; i <= 5; ++i) {

					Sleep(500);
					printf("CLIENT ITERATION N: %i\n", i);
					if (sendto(s, msg, bufferSize, 0, (const struct sockaddr*)&remoteAddr, sizeof(remoteAddr)) != SOCKET_ERROR) {

						printf("Client Sends: %s\n", msg);
						//struct sockaddr_in bindAddr;

						printf("Client Awaiting Response...\n");
						if (recvfrom(s, response, bufferSize, 0, nullptr, nullptr) != SOCKET_ERROR) {
							printf("Client Receives: %s\n", response);
						}
						else {
							iResult = WSAGetLastError();
							printf("Message Reception Error! Error code: %i\n", iResult);
						}

						strcpy_s(response, bufferSize, "");
					}
					else {
						iResult = WSAGetLastError();
						printf("Message Sending Error! Error code: %i\n", iResult);
					}

					printf("\n");
				}

				delete[] response;

				// In UDP we will make the client send a termination message to the server, as the server is not supervising if this client socket has closed
				printf("CLIENT TERMINATION\n");
				if (sendto(s, "TERMINATE\n", sizeof("TERMINATE\n"), 0, (const struct sockaddr*)&remoteAddr, sizeof(remoteAddr)) == SOCKET_ERROR) {
					iResult = WSAGetLastError();
					printf("Terminate Sending Error! Error code: %i\n", iResult);
				}
			}
			else {
				iResult = WSAGetLastError();
				printf("IP Text to Binary Error! Error code: %i\n", iResult);
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

	printf("Closing Client...\n\n");
	system("pause");
	return 0;
}