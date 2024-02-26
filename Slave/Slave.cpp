#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET server_sock = INVALID_SOCKET;
    struct sockaddr_in master_server_addr;
    int addr_len = sizeof(master_server_addr);

    // Initialize Winsock
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    master_server_addr.sin_family = AF_INET;
    master_server_addr.sin_port = htons(8080);
    
    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &master_server_addr.sin_addr) <= 0) {
        std::cerr << "\nInvalid address/ Address not supported \n";
        closesocket(server_sock);
        WSACleanup();
        return -1;
    }

    // Connect to the master server
    if (connect(server_sock, (struct sockaddr*)&master_server_addr, sizeof(master_server_addr)) < 0) {
        std::cerr << "\nConnection Failed \n";
        closesocket(server_sock);
        WSACleanup();
        return -1;
    }

    std::string identity = "SLAVE";
    send(server_sock, identity.c_str(), identity.size(), 0);

    char buffer[1024] = { 0 };
    //const char* slave_message = "message received";
    while (true) {
        result = recv(server_sock, buffer, sizeof(buffer), 0);
        if (result > 0) {
            buffer[result] = '\0'; // Null-terminate the received message
            std::cout << "Message from master: " << buffer << std::endl;

            //TODO: THIS IS WHERE THE PRIME CHECKING HAPPENS CHUCHUCHU//

            // Construct the response message
            std::string response_message = " Primes Checked";
            //response_message += buffer; // Append the received message to the response message

            // Send the response back to the master server
            send(server_sock, response_message.c_str(), response_message.length(), 0);
        }
        else if (result == 0) {
            std::cout << "Connection closed\n";
            break;
        }
        else {
            std::cerr << "recv failed with error: " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // Cleanup
    closesocket(server_sock);
    WSACleanup();
    return 0;
}
