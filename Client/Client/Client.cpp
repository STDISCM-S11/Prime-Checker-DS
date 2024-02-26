#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

int main() {
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in server;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    // Create the socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    if (inet_pton(AF_INET, "192.168.100.93", &server.sin_addr) <= 0) {
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
    const char* clientMsg = "CLIENT";
    send(sock, clientMsg, strlen(clientMsg), 0);

    // Here you can implement further communication logic as needed
    // For example, sending a specific task or receiving data from the server

    // Close the socket
    closesocket(sock);

    // Cleanup Winsock
    WSACleanup();

    return 0;
}