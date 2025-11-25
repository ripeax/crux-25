#include "modules/c2/comm.h"
#include <iostream>

int main() {
    Comm comm(L"Crux/1.0");
    
    if (!comm.Initialize()) {
        std::cerr << "Failed to initialize WinHTTP" << std::endl;
        return 1;
    }

    // Test GET request to httpbin.org
    std::cout << "Sending GET request to httpbin.org..." << std::endl;
    std::string response = comm.SendRequest(L"httpbin.org", 443, L"/get", L"GET");
    
    if (response.empty()) {
        std::cerr << "Failed to receive response" << std::endl;
        return 1;
    }

    std::cout << "Response received:" << std::endl;
    std::cout << response << std::endl;

    try {
        auto jsonResponse = json::parse(response);
        std::cout << "Parsed JSON successfully!" << std::endl;
        std::cout << "Origin: " << jsonResponse["origin"] << std::endl;
    } catch (json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
