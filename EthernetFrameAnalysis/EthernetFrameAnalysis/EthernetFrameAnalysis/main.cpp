#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <conio.h>

const uint16_t maxFrameLength = 0x05FE;
const uint16_t Ethernet_Raw_mark = 0xFFFF; // IPX
const uint16_t Ethernet_SNAP_mark = 0xAAAA;
const uint8_t bitesMAC = 6;

uint16_t numAll = 0;
uint16_t LLC_cnt = 0;
uint16_t Raw_cnt = 0;
uint16_t II_cnt = 0;
uint16_t SNAP_cnt = 0;

uint16_t IPv4_cnt = 0;
uint16_t ARP_cnt = 0;
uint16_t STP_cnt = 0;

int handleframe(std::ifstream& file);
int handleIP(std::ifstream& file);
int handleARP(std::ifstream& file);
int handleSTP(std::ifstream& file);

int main()
{
	char filename[32]/* = "ethers01.bin"*/;

	std::cout << "Enter filename: ";
	std::cin.getline(filename, 32);

	std::ifstream file(filename, std::ios::binary);

	if (file)
	{
		while (file.peek() != EOF)
		{
			numAll++;
			std::cout << std::dec << "\nFrame " << numAll << std::endl;
			handleframe(file);
		}
		file.close();

		std::cout << "\nFrames:" << std::dec << numAll
				  << "\n  Ethernet LLC: " << LLC_cnt
				  << "\n  Ethernet RAW: " << Raw_cnt
				  << "\n  Ethernet II: " << II_cnt
				  << "\n  Ethernet SNAP: " << SNAP_cnt << std::endl;

		std::cout
			<< "\n  IPV4: " << IPv4_cnt
			<< "\n  ARP: " << ARP_cnt
			<< "\n  STP: " << STP_cnt << std::endl;
	}
	else
		std::cerr << "Error opening file!" << std::endl;

	_getch();
	return 0;
}

// tools //
std::string uint32toIPv4(uint32_t ip)
{
	std::ostringstream oss;
	oss << ((ip      ) & 0xFF) << "." 
		<< ((ip >> 8 ) & 0xFF) << "."
		<< ((ip >> 16) & 0xFF) << "." 
		<< ((ip >> 24) & 0xFF);
	return oss.str();
}

std::string uint64toMAC(uint64_t mac)
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0') << std::setw(2)
		<< ((mac      ) & 0xFF) << std::setw(2)
		<< ((mac >> 8 ) & 0xFF) << std::setw(2)
		<< ((mac >> 16) & 0xFF) << std::setw(2)
		<< ((mac >> 24) & 0xFF) << std::setw(2)
		<< ((mac >> 32) & 0xFF) << std::setw(2)
		<< ((mac >> 40) & 0xFF);
	std::string mac_str = ss.str();
	for (int i = 10; i > 0; i -= 2) {
		mac_str.insert(i, ":");
	}
	return mac_str;
}

std::string uint64toSTPID(uint64_t id)
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0') << std::setw(2)
		<< ((id) & 0xFF) << std::setw(2)
		<< ((id >> 8) & 0xFF) << std::setw(2)
		<< ((id >> 16) & 0xFF) << std::setw(2)
		<< ((id >> 24) & 0xFF) << std::setw(2)
		<< ((id >> 32) & 0xFF) << std::setw(2)
		<< ((id >> 40) & 0xFF) << std::setw(2)
		<< ((id >> 48) & 0xFF) << std::setw(2)
		<< ((id >> 56) & 0xFF);
	std::string id_str = ss.str();
	for (int i = 14; i > 0; i -= 2) {
		id_str.insert(i, "-");
	}
	return id_str;
}

std::string uint32toOUI(uint32_t oui)
{
	std::ostringstream oss;
	oss << std::hex << std::setfill('0') << std::setw(2)
		<< ((oui      ) & 0xFF) << std::setw(2)
		<< ((oui >> 8 ) & 0xFF) << std::setw(2)
		<< ((oui >> 16) & 0xFF);
	return oss.str();
}
// /tools //

int handleframe(std::ifstream& file)
{
	uint64_t MAC;
	uint32_t OUI;
	uint16_t type_length, data_2_bytes, organization_type;
	uint8_t  LLC_3rd_bite;

	// MAC destination
	file.read(reinterpret_cast<char*>(&MAC), bitesMAC);
	if (file.gcount() != 6) {
		return 1;
	}
	std::cout << "MAC (Destination): " << uint64toMAC(MAC) << std::endl;

	// MAC source
	file.read(reinterpret_cast<char*>(&MAC), bitesMAC);
	std::cout << "MAC (Source): " << uint64toMAC(MAC) << std::endl;

	// frame type or length
	file.read(reinterpret_cast<char*>(&type_length), 2);
	type_length = (type_length << 8) | type_length >> 8;

	if (type_length > maxFrameLength) // frame type
	{
		std::cout << "Frame Type: Ethernet II/DIX (" << std::hex << std::cout.width(4) << type_length << ")" << std::endl;
		if (type_length == 0x0800)
		{
			handleIP(file);
			IPv4_cnt++;
		}
		if (type_length == 0x0806)
		{
			handleARP(file);
			ARP_cnt++;
		}
		II_cnt++;
	}
	else // frame lenght
	{
		// 2 bytes of data
		file.read(reinterpret_cast<char*>(&data_2_bytes), 2);
		data_2_bytes = data_2_bytes << 8 | data_2_bytes >> 8;

		if (data_2_bytes == Ethernet_Raw_mark) // 0xFFFF
		{
			std::cout << "Frame Type: Ethernet 802.3 Raw/Novell" << std::endl;
			std::cout << "   Length: " << type_length;
			handleIP(file);
			IPv4_cnt++;
			Raw_cnt++;
		}
		else if (data_2_bytes == Ethernet_SNAP_mark) //0xAAAA
		{
			std::cout << "Frame Type: Ethernet SNAP" << std::endl;
			std::cout << "   Length: " << type_length;

			file.read(reinterpret_cast<char*>(&LLC_3rd_bite), 1);
			std::cout << "   LLC: " << std::hex << ((data_2_bytes >> 8) & 0x00FF) << "-" << (data_2_bytes & 0x00FF) << "-" << +LLC_3rd_bite << std::endl;
			//file.seekg(1, std::ios_base::cur);
			
			file.read(reinterpret_cast<char*>(&OUI), 3);
			std::cout << "   Organization: " << uint32toOUI(OUI) << std::endl;

			file.read(reinterpret_cast<char*>(&organization_type), 2);
			organization_type = organization_type << 8 | organization_type >> 8;
			std::cout << "   Frame Type by Organization: " << std::hex << organization_type << std::endl;

			handleIP(file);
			IPv4_cnt++;
			SNAP_cnt++;
		}
		else // DSAP and SSAP
		{
			std::cout << "Frame Type: Ethernet 802.3/LLC" << std::endl;
			std::cout << "   Length: " << type_length;

			file.read(reinterpret_cast<char*>(&LLC_3rd_bite), 1);
			std::cout << "   LLC: " << std::hex << ((data_2_bytes >> 8) & 0x00FF) << "-" << (data_2_bytes & 0x00FF) << "-" << +LLC_3rd_bite << std::endl;

			if (data_2_bytes == 0x0606)
			{
				handleIP(file);
				IPv4_cnt++;
			}
			if (data_2_bytes == 0x4242)
			{
				handleSTP(file);
				STP_cnt++;
			}
			LLC_cnt++;
		}
	}

	return 0;
}

int handleIP(std::ifstream& file)
{
	uint8_t  IP_ver, IPV4_hlen,   IPV4_service;       uint16_t IPV4_total_length;
	uint16_t IPV4_identification; uint8_t IPV4_flags; uint16_t IPV4_offset;
	uint8_t	 IPV4_time_to_live,   IPV4_protocol;      uint16_t IPV4_header_checksum;
	uint32_t IPV4_source_address, 
			 IPV4_destination_address;

	file.read(reinterpret_cast<char*>(&IPV4_service), 1);
	if (file.gcount() != 1) {
		std::cout << "Unable to read version and hlen" << std::endl;
		return -1;
	}
	IP_ver = (IPV4_service & 0xF0) >> 4;
	IPV4_hlen = (IPV4_service & 0x0F) * 4;

	file.read(reinterpret_cast<char*>(&IPV4_service), 1);
	if (file.gcount() != 1) {
		std::cout << "Unable to read service" << std::endl;
		return -2;
	}

	file.read(reinterpret_cast<char*>(&IPV4_total_length), 2);
	if (file.gcount() != 2) {
		std::cout << "Unable to read total length" << std::endl;
		return -3;
	}
	IPV4_total_length = (IPV4_total_length << 8) | IPV4_total_length >> 8;

	file.read(reinterpret_cast<char*>(&IPV4_identification), 2);
	if (file.gcount() != 2) {
		std::cout << "Unable to read identification" << std::endl;
		return -4;
	}
	IPV4_identification = (IPV4_identification << 8) | IPV4_identification >> 8;

	file.read(reinterpret_cast<char*>(&IPV4_offset), 2);
	if (file.gcount() != 2) {
		std::cout << "Unable to read flags and fragmentation offser" << std::endl;
		return -5;
	}
	IPV4_offset = (IPV4_offset << 8) | IPV4_offset >> 8;
	IPV4_flags = (IPV4_offset & 0xE000) >> 13;
	IPV4_offset = IPV4_offset & 0x1FFF;

	file.read(reinterpret_cast<char*>(&IPV4_time_to_live), 1);
	if (file.gcount() != 1) {
		std::cout << "Unable to read time to live" << std::endl;
		return -6;
	}

	file.read(reinterpret_cast<char*>(&IPV4_protocol), 1);
	if (file.gcount() != 1) {
		std::cout << "Unable to read protocol" << std::endl;
		return -7;
	}

	file.read(reinterpret_cast<char*>(&IPV4_header_checksum), 2);
	if (file.gcount() != 2) {
		std::cout << "Unable to read header checksum" << std::endl;
		return -8;
	}
	IPV4_header_checksum = (IPV4_header_checksum << 8) | IPV4_header_checksum >> 8;

	file.read(reinterpret_cast<char*>(&IPV4_source_address), 4);
	if (file.gcount() != 4) {
		std::cout << "Unable to read source IP adress" << std::endl;
		return -9;
	}

	file.read(reinterpret_cast<char*>(&IPV4_destination_address), 4);
	if (file.gcount() != 4) {
		std::cout << "Unable to read destination IP address" << std::endl;
		return -10;
	}

	file.seekg(IPV4_total_length - IPV4_hlen, std::ios_base::cur);

	std::cout << "\tIP Version: " << std::dec << +IP_ver <<
		"\n\tIPV4 header length: " << +IPV4_hlen <<
		"\n\tIPV4 tos: " << +IPV4_service <<
		"\n\tIPV4 datagramm length: " << IPV4_total_length <<
		"\n\tIPV4 identification: " << IPV4_identification <<
		"\n\tIPV4 flags: " << +IPV4_flags <<
		"\n\tIPV4 offset: " << IPV4_offset <<
		"\n\tIPV4 time to live: " << +IPV4_time_to_live <<
		"\n\tIPV4 protocol: " << +IPV4_protocol <<
		"\n\tIPV4 header checksum: " << IPV4_header_checksum <<
		"\n\tIPV4 source address: " << uint32toIPv4(IPV4_source_address) <<
		"\n\tIPV4 destination address: " << uint32toIPv4(IPV4_destination_address) << std::endl;

	return 0;
}

int handleARP(std::ifstream& file)
{
	uint16_t ARP_hardware_type,           ARP_protocol_type;
	uint8_t  ARP_hlen, ARP_plen; uint16_t ARP_operation;
	uint64_t ARP_sender_hardware_address = 0,
			 ARP_sender_protocol_address = 0,
			 ARP_target_hardware_address = 0,
			 ARP_target_protocol_address = 0;

	file.read(reinterpret_cast<char*>(&ARP_hardware_type), 2);
	ARP_hardware_type = (ARP_hardware_type << 8) | ARP_hardware_type >> 8;

	file.read(reinterpret_cast<char*>(&ARP_protocol_type), 2);
	ARP_protocol_type = (ARP_protocol_type << 8) | ARP_protocol_type >> 8;

	file.read(reinterpret_cast<char*>(&ARP_hlen), 1);

	file.read(reinterpret_cast<char*>(&ARP_plen), 1);

	file.read(reinterpret_cast<char*>(&ARP_operation), 2);
	ARP_operation = (ARP_operation << 8) | ARP_operation >> 8;

	file.read(reinterpret_cast<char*>(&ARP_sender_hardware_address), ARP_hlen);

	file.read(reinterpret_cast<char*>(&ARP_sender_protocol_address), ARP_plen);

	file.read(reinterpret_cast<char*>(&ARP_target_hardware_address), ARP_hlen);

	file.read(reinterpret_cast<char*>(&ARP_target_protocol_address), ARP_plen);

	std::cout << "\tHardware type: " << std::hex << static_cast<int>(ARP_hardware_type)
		<< "\n\tProtocol type: " << static_cast<int>(ARP_protocol_type)
		<< "\n\tHardware length: " << std::dec << static_cast<int>(ARP_hlen)
		<< "\n\tProtocol length: " << static_cast<int>(ARP_plen)
		<< "\n\tOperation: " << std::hex << static_cast<int>(ARP_operation)
		<< "\n\tSender hardware address: " << (ARP_hlen == 0x6 ? uint64toMAC(ARP_sender_hardware_address) : "")
		<< "\n\tSender protocol address: " << (ARP_plen == 0x4 ? uint32toIPv4(static_cast<uint32_t>(ARP_sender_protocol_address)) : "")
		<< "\n\tTarget hardware address: " << (ARP_hlen == 0x6 ? uint64toMAC(ARP_target_hardware_address) : "")
		<< "\n\tTarget protocol address: " << (ARP_plen == 0x4 ? uint32toIPv4(static_cast<uint32_t>(ARP_target_protocol_address)) : "") << std::endl;

	return 0;
}

int handleSTP(std::ifstream& file)
{
	uint16_t STP_protocol_id;
	uint8_t  STP_protocol_version,
			 BPDU_type,
			 STP_flags;
	uint64_t STP_root_id;
	uint32_t STP_root_path_cost;
	uint64_t STP_bridge_id;
	uint16_t STP_port_id,
			 STP_message_age,
			 STP_maximum_age, 
			 STP_hello_time, 
			 STP_forward_delay;

	if (!file.read(reinterpret_cast<char*>(&STP_protocol_id), 2)) {
		std::cout << "Unable to read protocol identifier" << std::endl;
		return -1;
	}
	STP_protocol_id = (STP_protocol_id << 8) | STP_protocol_id >> 8;

	if (!file.read(reinterpret_cast<char*>(&STP_protocol_version), 1)) {
		std::cout << "Unable to read version" << std::endl;
		return -2;
	}

	if (!file.read(reinterpret_cast<char*>(&BPDU_type), 1)) {
		std::cout << "Unable to read message type" << std::endl;
		return -3;
	}

	if (!file.read(reinterpret_cast<char*>(&STP_flags), 1)) {
		std::cout << "Unable to read flags" << std::endl;
		return -4;
	}

	if (!file.read(reinterpret_cast<char*>(&STP_root_id), 8)) {
		std::cout << "Unable to read root id" << std::endl;
		return -5;
	}

	if (!file.read(reinterpret_cast<char*>(&STP_root_path_cost), 4)) {
		std::cout << "Unable to read root path cost" << std::endl;
		return -6;
	}
	STP_root_path_cost = 
		((STP_root_path_cost >> 24) & 0xFF) |
		((STP_root_path_cost >> 8) & 0xFF00) |
		((STP_root_path_cost << 8) & 0xFF0000) |
		((STP_root_path_cost >> 24) & 0xFF000000);

	if (!file.read(reinterpret_cast<char*>(&STP_bridge_id), 8)) {
		std::cout << "Unable to read bridge id" << std::endl;
		return -7;
	}

	if (!file.read(reinterpret_cast<char*>(&STP_port_id), 2)) {
		std::cout << "Unable to read port id" << std::endl;
		return -8;
	}
	STP_port_id = (STP_port_id << 8) | STP_port_id >> 8;

	if (!file.read(reinterpret_cast<char*>(&STP_message_age), 2)) {
		std::cout << "Unable to read message age" << std::endl;
		return -9;
	}
	STP_message_age = (STP_message_age << 8) | STP_message_age >> 8;

	if (!file.read(reinterpret_cast<char*>(&STP_maximum_age), 2)) {
		std::cout << "Unable to read maximum age" << std::endl;
		return -10;
	}
	STP_message_age = (STP_message_age << 8) | STP_message_age >> 8;

	if (!file.read(reinterpret_cast<char*>(&STP_hello_time), 2)) {
		std::cout << "Unable to read hello time" << std::endl;
		return -11;
	}
	STP_hello_time = (STP_hello_time << 8) | STP_hello_time >> 8;

	if (!file.read(reinterpret_cast<char*>(&STP_forward_delay), 2)) {
		std::cout << "Unable to read forward delay" << std::endl;
		return -12;
	}
	STP_forward_delay = (STP_forward_delay << 8) | STP_forward_delay >> 8;

	std::cout << "\tSTP protocol id: " << std::dec << STP_protocol_id
		<< "\n\tSTP protocol version: " << +STP_protocol_version
		<< "\n\tBPDU type: " << +BPDU_type
		<< "\n\tSTP flags: " << +STP_flags
		<< "\n\tSTP root id: " << uint64toSTPID(STP_root_id)
		<< "\n\tSTP root path cost: " << STP_root_path_cost
		<< "\n\tSTP bridge id: " << uint64toSTPID(STP_bridge_id)
		<< "\n\tSTP port id: " << STP_port_id
		<< "\n\tSTP message age: " << STP_message_age
		<< "\n\tSTP maximum age: " << STP_maximum_age
		<< "\n\tSTP hello time: " << STP_hello_time
		<< "\n\tSTP forward delay: " << STP_forward_delay << std::endl;

	return 0;
}