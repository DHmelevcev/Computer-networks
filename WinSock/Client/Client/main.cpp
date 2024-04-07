#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#ifdef max
#undef max
#endif

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 2001

#define TEXT_SIZE 1024

int main()
{
	char HOST[16]{'\0'};
	u_short PORT = 0;
	std::cout << "Enter Host: " << std::endl;
	std::cin.getline(HOST, 16);
	std::cout << "Enter Port: " << std::endl;
	std::cin >> PORT;
	if (!std::cin)
	{
		std::cout << "Invalid Port!" << std::endl;
		return 1;
	}
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	std::cin.clear();

	if (!strlen(HOST))
	{
		char buffer[16]{ DEFAULT_HOST };
		memcpy_s(HOST, 16, buffer, 16);
	}

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

	// Initialize socket
	SOCKET sConnection = socket(AF_INET, SOCK_STREAM, NULL);
	if (sConnection == INVALID_SOCKET)
	{
		std::cout << "Error at socket(): " << WSAGetLastError() << std::endl;
		WSACleanup();
		return 1;
	}

	// Initialize address
	SOCKADDR_IN addr;
	inet_pton(AF_INET, HOST, &addr.sin_addr.s_addr);
	addr.sin_port = htons(PORT);
	addr.sin_family = AF_INET;
	int addrSize = sizeof(addr);

	// Connection
	iResult = connect(sConnection, (LPSOCKADDR)&addr, addrSize);
	if (iResult == SOCKET_ERROR)
	{
		std::cout << "Connection failed with error : " << WSAGetLastError() << std::endl;
		closesocket(sConnection);
		WSACleanup();
		return 1;
	}

	char message[256];
	recv(sConnection, message, sizeof(message), NULL);
	std::cout << message;

	char text[TEXT_SIZE]{};
	size_t textsize = 0;
	char buffer[TEXT_SIZE + 1];
	while (true)
	{
		std::cin.getline(buffer, TEXT_SIZE + 1);
		if (!buffer[0])
		{
			textsize++;
			break;
		} 
		if (textsize > 0)
		{
			text[textsize++] = '\n';
		}

		size_t buffersize = strlen(buffer);
		if (textsize + buffersize > sizeof(text) - 1)
		{
			while (true)
			{
				std::cin.getline(buffer, TEXT_SIZE + 1);
				if (!buffer[0])
				{
					break;
				}
			}
			std::cout << "Text too big\nTry again : \n\n";
			memset(text, '\0', textsize);
			textsize = 0;
			continue;
		}

		memcpy_s(text + textsize, TEXT_SIZE - 1 - textsize, buffer, buffersize);
		textsize += buffersize;
		text[textsize] = '\0';
	}

	send(sConnection, text, textsize, NULL);

	recv(sConnection, message, sizeof(message), NULL);
	std::cout << message;
	recv(sConnection, text, TEXT_SIZE, NULL);
	std::cout << text << std::endl;

	std::cin.getline(HOST, 1);
	closesocket(sConnection);
	WSACleanup();

	return 0;
}