#include <boost/asio.hpp>
#include <iostream>

int main() {
    try {
        boost::asio::io_context io;

        // Create UDP socket and bind to all interfaces on port 9000
        boost::asio::ip::udp::socket socket(io,
            boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), 9000));

        std::cout << "UDP server is listening on port 9000...\n";

        char buffer[1024];
        boost::asio::ip::udp::endpoint sender_endpoint;

        while (true) {
            // Wait for a message
            size_t bytes_received = socket.receive_from(
                boost::asio::buffer(buffer), sender_endpoint);

            std::string msg(buffer, bytes_received);

            std::cout << "Received from " << sender_endpoint.address().to_string()
                << ": " << msg << "\n";
        }

    }
    catch (std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }

    return 0;
}
