#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <chrono>

#pragma comment(lib, "Ws2_32.lib")

int main()
{
    WSADATA wsaData;
    SOCKET sock = INVALID_SOCKET;
    struct sockaddr_in server;

    // Initialize Winsock
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0)
    {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // Create the socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    if (inet_pton(AF_INET, "127.0.0.1", &server.sin_addr) <= 0)
    {
        std::cerr << "Invalid address/ Address not supported \n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    server.sin_family = AF_INET;
    server.sin_port = htons(8080);

    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        std::cerr << "Connect failed with error.\n";
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server.\n";

    std::string identity = "CLIENT";
    send(sock, identity.c_str(), identity.size(), 0);

    char buffer[1024] = {0};
    char startMsg[1024] = {0};
    char endMsg[1024] = {0};
    std::string msg;

    std::cout << "start point: ";
    std::cin >> startMsg;
    msg += startMsg;
    msg += ",";
    std::cout << "end point: ";
    std::cin >> endMsg;
    msg += endMsg;

    // Start timing
    auto start = std::chrono::high_resolution_clock::now();

    send(sock, msg.c_str(), msg.size(), 0);

    int bytesReceived = recv(sock, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0)
    {
        buffer[bytesReceived] = '\0';
        std::cout << "Response from master: " << buffer << std::endl;
    }

    // End timing
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    double duration_seconds = duration.count() / 1000.0;
    std::cout << "Response time: " << duration_seconds << " seconds.\n";

    closesocket(sock);

    WSACleanup();

    return 0;
}
