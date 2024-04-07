#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#ifdef max
#undef max
#endif

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 2001

#define TEXT_SIZE 1024

size_t TextHandler(char* text, size_t textsize)
{
	size_t resultsize = 0;
	char buffer[TEXT_SIZE]{};

	UINT32 counter = 0;
	for (size_t i = 0; i < textsize; i++)
	{
		if (TEXT_SIZE == resultsize)
		{
			break;
		}

		if (TEXT_SIZE == resultsize - 1)
		{
			buffer[resultsize++] = '\0';
			break;
		}

		if (!counter && text[i] == ' ')
		{
			buffer[resultsize++] = ' ';
			continue;
		}

		if (!counter && text[i] == '\n')
		{
			buffer[resultsize++] = '\n';
			continue;
		}

		if (text[i] == '.' || text[i] == '!' || text[i] == '?')
		{
			std::string str = std::to_string(counter);
			const char* count = str.c_str();
			size_t countsize = strlen(count);
			if (TEXT_SIZE - resultsize < countsize + 4)
			{
				buffer[resultsize++] = text[i];
				buffer[resultsize++] = '\0';
				break;
			}
			buffer[resultsize++] = text[i];
			buffer[resultsize++] = '(';
			memcpy_s(buffer + resultsize, TEXT_SIZE - resultsize - countsize, count, countsize);
			resultsize += countsize;
			buffer[resultsize++] = ')';
			counter = 0;
			continue;
		}

		if (text[i] == '\0')
		{
			if (!counter)
			{
				buffer[resultsize++] = '\0';
				break;
			}

			std::string str = std::to_string(counter);
			const char* count = str.c_str();
			size_t countsize = strlen(count);
			if (TEXT_SIZE - resultsize < countsize + 3)
			{
				buffer[resultsize++] = '\0';
				break;
			}
			buffer[resultsize++] = '(';
			memcpy_s(buffer + resultsize, TEXT_SIZE - resultsize - countsize, count, countsize);
			resultsize += countsize;
			buffer[resultsize++] = ')';
			buffer[resultsize++] = '\0';
			break;
		}

		buffer[resultsize++] = text[i];
		counter++;
	}

	memcpy_s(text, TEXT_SIZE, buffer, resultsize);

	return resultsize;
}

void ClientHandler(SOCKET sClient)
{
	char messageIn[] = "\nYou connected...\nEnter text to receive length of each sentence:\n\n";
	char messageOut[] = "Text with length of each sentence:\n\n";
	char text[TEXT_SIZE];
	send(sClient, messageIn, sizeof(messageIn), NULL);
	recv(sClient, text, TEXT_SIZE, NULL);
	size_t textsize = TextHandler(text, TEXT_SIZE);
	send(sClient, messageOut, sizeof(messageOut), NULL);
	send(sClient, text, textsize, NULL);
}

int main(int argc, char* argv[])
{
	u_short PORT = 0;
	std::cout << "Enter Port: " << std::endl;
	std::cin >> PORT;
	if (!std::cin)
	{
		std::cout << "Invalid Port!" << std::endl;
		return 1;
	}
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	std::cin.clear();
	if (!PORT)
	{
		PORT = DEFAULT_PORT;
	}

	// Initialize Winsock
	WSAData wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 1), &wsaData);
	if (iResult != 0)
	{
		std::cout << "WSAStartup failed: " << iResult << std::endl;
		return 1;
	}

	char ac[80];
	if (gethostname(ac, sizeof(ac)) == SOCKET_ERROR) {
		std::cout << "Error " << WSAGetLastError() <<
			" when getting local host name.\n";
		return 1;
	}
	struct hostent* phe = gethostbyname(ac);
	if (phe == 0) {
		std::cout << "Yow! Bad host lookup.\n";
		return 1;
	}

	for (int i = 0; phe->h_addr_list[i] != 0; ++i) {
		struct in_addr addr;
		memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
		std::cout << "Server Address " << i << ": " << inet_ntoa(addr) << std::endl;
	}

	// Initialize socket
	SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
	if (sListen == INVALID_SOCKET)
	{
		std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	// Initialize address
	SOCKADDR_IN addr;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(PORT);
	addr.sin_family = AF_INET;
	int addrSize = sizeof(addr);

	// Setup the listening socket
	iResult = bind(sListen, (LPSOCKADDR)&addr, addrSize);
	if (iResult == SOCKET_ERROR) {
		std::cout << "Bind failed with error : " << WSAGetLastError() << std::endl;
		closesocket(sListen);
		WSACleanup();
		return 1;
	}

	std::cout << "\nServer started on port: " << PORT << "\n\n";

	// Setup the listen
	if (listen(sListen, SOMAXCONN) == SOCKET_ERROR) {
		std::cout << "Listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(sListen);
		WSACleanup();
		return 1;
	}
	bool running = true;

	while (running)
	{
		SOCKET sClient;
		sClient = accept(sListen, (LPSOCKADDR)&addr, &addrSize);
		if (sClient == INVALID_SOCKET)
		{
			std::cout << "Connection error: " << WSAGetLastError() << std::endl;
		}

		char str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(addr.sin_addr), str, INET_ADDRSTRLEN);
		std::cout << "Client connected " << str << '(' << addr.sin_port << ')' << std::endl;
		ClientHandler(sClient);
		closesocket(sClient);
	}

	closesocket(sListen);
	WSACleanup();

	return 0;
}