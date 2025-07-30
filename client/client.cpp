#include <boost/asio.hpp>
#include <iostream>
#include <thread>
#include <vector>
#include <array>

#ifdef _WIN32
#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment(lib, "iphlpapi.lib")
#else
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

std::vector<std::string> get_broadcast_addresses() {
    std::vector<std::string> broadcasts;

#ifdef _WIN32
    ULONG buf_len = 15000;
    std::vector<char> buffer(buf_len);
    IP_ADAPTER_ADDRESSES* adapter_addresses = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    DWORD ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, adapter_addresses, &buf_len);
    if (ret != NO_ERROR) {
        std::cerr << "GetAdaptersAddresses failed\n";
        return broadcasts;
    }

    for (IP_ADAPTER_ADDRESSES* adapter = adapter_addresses; adapter; adapter = adapter->Next) {
        if (adapter->OperStatus != IfOperStatusUp || !adapter->FirstUnicastAddress)
            continue;

        for (IP_ADAPTER_UNICAST_ADDRESS* addr = adapter->FirstUnicastAddress; addr; addr = addr->Next) {
            SOCKADDR_IN* sa_in = reinterpret_cast<SOCKADDR_IN*>(addr->Address.lpSockaddr);
            DWORD prefix_len = addr->OnLinkPrefixLength;
            ULONG ip = ntohl(sa_in->sin_addr.S_un.S_addr);

            ULONG mask = 0xFFFFFFFF << (32 - prefix_len);
            ULONG broadcast = ip | ~mask;

            in_addr bcast_addr;
            bcast_addr.S_un.S_addr = htonl(broadcast);
            broadcasts.push_back(inet_ntoa(bcast_addr));
        }
    }
#else
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return broadcasts;
    }

    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (!(ifa->ifa_flags & IFF_UP) || !(ifa->ifa_flags & IFF_BROADCAST) || !ifa->ifa_broadaddr)
            continue;

        if (ifa->ifa_broadaddr->sa_family == AF_INET) {
            struct sockaddr_in* sa = reinterpret_cast<struct sockaddr_in*>(ifa->ifa_broadaddr);
            broadcasts.push_back(inet_ntoa(sa->sin_addr));
        }
    }
    freeifaddrs(ifaddr);
#endif

    return broadcasts;
}

int main() {
    try {
        boost::asio::io_context io;
        boost::asio::ip::udp::socket socket(io);
        socket.open(boost::asio::ip::udp::v4());
        socket.set_option(boost::asio::socket_base::broadcast(true));

        std::string message = " Hello from client!";
        unsigned short port = 9000;

        auto broadcast_ips = get_broadcast_addresses();
        if (broadcast_ips.empty()) {
            std::cerr << " No broadcast addresses found.\n";
            return 1;
        }

        for (const auto& ip : broadcast_ips) {
            boost::asio::ip::udp::endpoint endpoint(
                boost::asio::ip::make_address_v4(ip), port);

            for (int i = 0; i < 3; ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                socket.send_to(boost::asio::buffer(message), endpoint);
                std::cout << " Sent to " << ip << " (attempt " << i + 1 << ")\n";
            }
        }

    }
    catch (std::exception& e) {
        std::cerr << " Client error: " << e.what() << "\n";
    }

    return 0;
}
