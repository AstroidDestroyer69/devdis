#include <boost/asio.hpp>
#include <iostream>
#include <vector>
#include <iphlpapi.h>
#include <thread>
#include <chrono>
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

std::vector<boost::asio::ip::address_v4> get_broadcast_addresses() {
    std::vector<boost::asio::ip::address_v4> result;

    ULONG size = 0;
    GetAdaptersAddresses(AF_INET, 0, nullptr, nullptr, &size);

    std::vector<unsigned char> buffer(size);
    IP_ADAPTER_ADDRESSES* adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());

    if (GetAdaptersAddresses(AF_INET, 0, nullptr, adapters, &size) == NO_ERROR) {
        for (IP_ADAPTER_ADDRESSES* adapter = adapters; adapter != nullptr; adapter = adapter->Next) {
            if (adapter->OperStatus != IfOperStatusUp) continue;

            IP_ADAPTER_UNICAST_ADDRESS* unicast = adapter->FirstUnicastAddress;
            while (unicast) {
                SOCKADDR_IN* sa_in = reinterpret_cast<SOCKADDR_IN*>(unicast->Address.lpSockaddr);
                DWORD ip = sa_in->sin_addr.S_un.S_addr;
                DWORD mask = 0xFFFFFFFF << (32 - unicast->OnLinkPrefixLength);
                DWORD broadcast = (ip & mask) | (~mask);

                boost::asio::ip::address_v4 bcast_addr(htonl(broadcast));
                result.push_back(bcast_addr);

                unicast = unicast->Next;
            }
        }
    }

    return result;
}

int main() {
    try {
        boost::asio::io_context io;

        boost::asio::ip::udp::socket socket(io);
        socket.open(boost::asio::ip::udp::v4());

        // Enable broadcast
        socket.set_option(boost::asio::socket_base::broadcast(true));

        std::string message = "Hello from client";
        unsigned short port = 9000;

        std::vector<boost::asio::ip::address_v4> broadcast_addresses = get_broadcast_addresses();
        int i = 0;
        for (const auto& bcast : broadcast_addresses) {
            while (i < 100) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                boost::asio::ip::udp::endpoint endpoint(bcast, port);
                socket.send_to(boost::asio::buffer(message), endpoint);
                std::cout << "Sent to: " << bcast.to_string()<<" Request No=>" << i << " " << std::endl;
                i++;
            }
        }

        socket.close();
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
    }

    return 0;
}
