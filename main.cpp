#include <iostream>
#include <string>
#include <array>
#include <memory>

std::string runCommand(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");

    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

int main() {
    std::string output = runCommand("netsh wlan show networks mode=bssid");
    std::cout << output << std::endl;
    return 0;
}
