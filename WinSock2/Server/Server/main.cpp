#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#ifdef max
#undef max
#endif

#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT 2001

#define MAX_CONNECTIONS 5
#define USER_NAME_LENGTH 33
#define TEXT_SIZE 1024

struct USER
{
	SOCKET sock;
	char ip[INET_ADDRSTRLEN];
	char name[USER_NAME_LENGTH];

} Users[MAX_CONNECTIONS];

size_t user_counter = 0;
size_t next_connection = 0;

void ClientHandler(u_int index)
{
	char message[TEXT_SIZE] = { "You connected to server.\n" };
	send(Users[index].sock, message, 26, NULL);

	char header[USER_NAME_LENGTH + 2]{};
	memcpy_s(header, USER_NAME_LENGTH, Users[index].name, USER_NAME_LENGTH);
	size_t name_lenght = strlen(header);
	memcpy_s(header + name_lenght, USER_NAME_LENGTH + 2 - name_lenght, { ": " }, 3);
	while (true)
	{
		if (recv(Users[index].sock, message, TEXT_SIZE, NULL) == SOCKET_ERROR)
		{
			closesocket(Users[index].sock);
			Users[index].sock = 0;
			user_counter--;
			return;
		}

		std::cout << header << message << std::endl;

		for (auto& user : Users)
		{
			if (user.sock != Users[index].sock && Users[index].sock)
			{
				send(user.sock, header, name_lenght + 3, NULL);
				send(user.sock, message, strlen(message) + 1, NULL);
			}
		}
	}
}

int main(int argc, char* argv[])
{
	u_short PORT = DEFAULT_PORT;
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

	while (true)
	{
		SOCKET sClient;

		while (user_counter == MAX_CONNECTIONS) {}

		sClient = accept(sListen, (LPSOCKADDR)&addr, &addrSize);
		if (sClient == INVALID_SOCKET)
		{
			std::cout << "Connection error: " << WSAGetLastError() << std::endl;
			continue;
		}
		if (next_connection == MAX_CONNECTIONS)
		{
			next_connection = 0;
			while (Users[next_connection].sock)
				next_connection++;
		}

		char user_name[USER_NAME_LENGTH];
		recv(sClient, user_name, USER_NAME_LENGTH, NULL);

		char user_ip[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &addr.sin_addr, user_ip, INET_ADDRSTRLEN);

		Users[next_connection].sock = sClient;
		memcpy_s(Users[next_connection].ip, INET_ADDRSTRLEN, user_ip, INET_ADDRSTRLEN);
		memcpy_s(Users[next_connection].name, USER_NAME_LENGTH, user_name, USER_NAME_LENGTH);

		char message[96]{};
		strcat_s(message, "Connected ");
		strcat_s(message, user_ip);
		strcat_s(message, " (");
		sprintf_s(user_ip, "%d", addr.sin_port);
		strcat_s(message, user_ip);
		strcat_s(message, "): ");
		strcat_s(message, user_name);
		strcat_s(message, "\n");
		std::cout << message << std::endl;
		for (auto& user : Users)
		{
			if (user.sock != sClient && user.sock)
			{
				send(user.sock, message, strlen(message) + 1, NULL);
			}
		}

		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ClientHandler, (LPVOID)next_connection, NULL, NULL);
		user_counter++;
		next_connection++;
	}

	closesocket(sListen);
	WSACleanup();

	return 0;
}