#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <ws2tcpip.h>
#include <signal.h>
#include <string>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

const WORD ICMP_DATA_SIZE = 32;
const WORD timeout = 3000;
WORD number_of_packets = 4;
unsigned char TTL = 60;

typedef struct _IP_OPTION_INFORMATION
{
	unsigned char       TTL;
	unsigned char       service;
	unsigned char       flags;
	unsigned char       options_size;
	unsigned char*		options_data;
} IP_OPTION_INFORMATION;

typedef struct _ICMP_ECHO_REPLY
{
	unsigned long         address;
	unsigned long         status;
	unsigned long         trip_time;
	unsigned short        data_size;
	unsigned short        reserved;
	void* data;
	IP_OPTION_INFORMATION options;
} ICMP_ECHO_REPLY;

typedef HANDLE(WINAPI* LP_ICMP_create_file)
(VOID);

typedef BOOL(WINAPI* LP_ICMP_close_handle)
(HANDLE ICMPhandle);

typedef DWORD(WINAPI* LP_ICMP_send_echo)
(
	HANDLE ICMPhandle,
	unsigned long destination_address,
	LPVOID request_data,
	WORD request_size,
	IP_OPTION_INFORMATION* request_options,
	LPVOID reply_buffer,
	DWORD reply_size,
	DWORD timeout
);

int main(int argc, char* argv[])
{
	setlocale(LC_ALL, "Ru");

	if (argc == 1)
	{
		std::cout << "Enter host name: ";
		char temp[256];
		std::cin.getline(temp, sizeof(temp));
		argv[1] = temp;
	}
	else if (argc == 4 && strcmp(argv[2], "-n") == 0)
	{
		number_of_packets = abs(std::stoi(argv[3]));
	}
	else if (argc == 4 && strcmp(argv[2], "-i") == 0)
	{
		TTL = abs(std::stoi(argv[3]));
	}
	else if (argc == 6 && strcmp(argv[2], "-n") == 0 && strcmp(argv[4], "-i") == 0)
	{
		number_of_packets = abs(std::stoi(argv[3]));
		TTL = abs(std::stoi(argv[5]));
	}
	else if (argc == 6 && strcmp(argv[2], "-i") == 0 && strcmp(argv[4], "-n") == 0)
	{
		number_of_packets = abs(std::stoi(argv[5]));
		TTL = abs(std::stoi(argv[3]));
	}
	else if (argc != 2)
	{
		std::cout << std::endl << "Usage: " << argv[0] << " target [Parameters]" << std::endl << std::endl;
		std::cout << "Parameters: " << std::endl;
		std::cout << "    -n \tMaximum packets to send." << std::endl;
		std::cout << "    -i TTL\t\tTime to live." << std::endl;
		return 1;
	}
		
	// Initialize Winsock
	WSAData wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		std::cout << "WSAStartup failed: " << iResult << std::endl;
		return 1;
	}

	// Get address
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	struct addrinfo* result;
	iResult = getaddrinfo(argv[1], NULL, &hints, &result);
	if (iResult != 0)
	{
		std::cout << "Unable to resolve name " << argv[1] << std::endl;
		return 1;
	}
	unsigned long dest_ip = reinterpret_cast<struct sockaddr_in*>(result->ai_addr)->sin_addr.s_addr;
	freeaddrinfo(result);

	// Load ICMP
	HINSTANCE ICMPlib = LoadLibrary(L"ICMP.dll");
	if (!ICMPlib)
	{
		std::cout << "Could not load up the ICMP DLL";
		WSACleanup();
		return -1;
	}

	// Load ICMP handle and functions
	LP_ICMP_create_file FUNCP_create_file = reinterpret_cast<LP_ICMP_create_file>(GetProcAddress(ICMPlib, "IcmpCreateFile"));
	LP_ICMP_close_handle FUNCP_close_handle = reinterpret_cast<LP_ICMP_close_handle>(GetProcAddress(ICMPlib, "IcmpCloseHandle"));
	LP_ICMP_send_echo FUNCP_send_echo = reinterpret_cast<LP_ICMP_send_echo>(GetProcAddress(ICMPlib, "IcmpSendEcho"));

	if (FUNCP_create_file == NULL || FUNCP_send_echo == NULL || FUNCP_close_handle == NULL)
	{
		std::cout << "Could not find ICMP functions in the ICMP DLL";
		WSACleanup();
		return 1;
	}

	// Create handle
	HANDLE handleICMP = FUNCP_create_file();
	if (handleICMP == INVALID_HANDLE_VALUE)
	{
		std::cout << "Could not get a valid ICMP handle";
		WSACleanup();
		return 1;
	}


	// Start PING
	std::cout << "\nОбмен пакетами с " << argv[1] << " [" << inet_ntoa(*reinterpret_cast<in_addr*>(&dest_ip)) << "] c " 
		<< ICMP_DATA_SIZE << " байтами данных" << std::endl;

	char ICMP_send_data[ICMP_DATA_SIZE];
	memset(ICMP_send_data, 'A', sizeof(ICMP_send_data));

	char ICMP_reply_data[sizeof(ICMP_ECHO_REPLY) + ICMP_DATA_SIZE];
	ICMP_ECHO_REPLY* P_echo_reply = reinterpret_cast<ICMP_ECHO_REPLY*>(ICMP_reply_data);

	IP_OPTION_INFORMATION IPOption;
	memset(&IPOption, 0, sizeof(IP_OPTION_INFORMATION));
	IPOption.TTL = TTL;

	WORD packets_recived = 0;
	WORD packets_lost = 0;
	unsigned long t = 0;
	unsigned long t_min = -1;
	unsigned long t_max = 0;
	while (packets_recived + packets_lost < number_of_packets)
	{
		DWORD response = FUNCP_send_echo(handleICMP, dest_ip, ICMP_send_data, sizeof(ICMP_send_data), &IPOption, ICMP_reply_data, sizeof(ICMP_reply_data), timeout);
		if (response != 0)
		{
			std::cout << "Ответ от " << inet_ntoa(*reinterpret_cast<in_addr*>(&P_echo_reply->address));
			++packets_recived;

			if (P_echo_reply->status == 0)
			{
				std::cout << ": число байт=" << static_cast<int>(P_echo_reply->data_size)
					<< " время=" << (P_echo_reply->trip_time == 0 ? "<1мс" : std::to_string(P_echo_reply->trip_time) + "мс")
					<< " TTL=" << +P_echo_reply->options.TTL << std::endl;

				t_min = t_min < P_echo_reply->trip_time ? t_min : P_echo_reply->trip_time;
				t_max = t_max > P_echo_reply->trip_time ? t_max : P_echo_reply->trip_time;
				t += P_echo_reply->trip_time;
			}
			else
				std::cout << ": Превышен срок жизни (TTL) при передаче пакета." << std::endl;
		}
		else
			++packets_lost;
	}

	// Statistics
	std::cout << "\nСтатистика Ping для " << inet_ntoa(*reinterpret_cast<in_addr*>(&dest_ip))
		<< ":\n\tПакетов: отправленно = " << number_of_packets
		<< ", получено = " << packets_recived
		<< ", потеряно = " << packets_lost
		<< "\n\t(" << 100.f * packets_lost / number_of_packets << "%) потерь"
		<< "\nПриблизительное время приема - передачи в мс:\n\tМинимальное = " << t_min
		<< "мсек, Максимальное = " << t_max << " мсек, Среднее = " << round(static_cast<float>(t) / number_of_packets) << std::endl;

	FUNCP_close_handle(handleICMP);
	WSACleanup();
	return 0;
}