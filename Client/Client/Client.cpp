#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in server;

    // Initialize Winsock
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // Create the socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported \n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(8080); // Server Port

    // Connect to remote server
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "Connect failed with error.\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server.\n";

    // Send an initial message
    std::string identity = "CLIENT";
    send(sock, identity.c_str(), identity.size(), 0);

    char buffer[1024] = { 0 };
    const char* clientMsg = "send to master";
    char msg[1024] = { 0 };
    
    // get input then send input to master
    std::cout << "message:" << std::endl;
    std::cin >> msg;
    send(sock, msg, strlen(msg), 0);
    recv(sock, buffer, sizeof(buffer) - 1, 0);

    // Fix Printing of message
    int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if(bytesReceived > 0) {
        buffer[bytesReceived] = '\0'; // Ensure null-termination
        std::cout << "Server: " << buffer << std::endl; // Print the received message
    }

    // Close the socket
    closesocket(sock);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}
