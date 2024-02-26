// Prime-Checker-DS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <thread>

#pragma comment (lib, "Ws2_32.lib")

void handle_slave(int socket)
{
    char buffer[1024] = { 0 };
    const char* message = "hello";
    send(socket, message, strlen(message), 0);
    std::cout << "Message sent to slave server\n";
    recv(socket, buffer, 1024, 0); // Example of reading a message from slave
    std::cout << "Message from slave: " << buffer << std::endl;
}

void handle_client(int socket)
{
    char buffer[1024] = { 0 };
    while (true)
    {
        int val = recv(socket, buffer, sizeof(buffer), 0);
        if (val <= 0)
        {
            std::cerr << "Error" << std::endl;
            closesocket(socket);
            break;
        }

        std::cout << buffer << std::endl;
    }
}

int main()
{
    WSADATA wsaData;
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = { 0 };
    const char* greeting = "Hello from server";

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed.\n");
        return 1;
    }

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Attach socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) == SOCKET_ERROR) {
        printf("setsockopt failed with error: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Bind the socket to the IP/port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Bind failed with error: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    //while (true) {
    //    int new_conn = accept(server_fd, (struct sockaddr*)&address, &addrlen);

    //    if (new_conn < 0 /*== INVALID_SOCKET*/) {
    //        printf("accept failed with error: %d\n", WSAGetLastError());
    //        continue;
    //        /*closesocket(server_fd);
    //        WSACleanup();
    //        return 1;*/
    //    }

    //    char buffer[1024] = {0};

    //    recv(new_conn, buffer, sizeof(buffer), 0);

    //    std::string identity = buffer;

    //    if (identity == "CLIENT")
    //    {
    //        std::cout << "Client connected" << std::endl;
    //        std::thread clientThread(handle_client, new_conn);
    //        clientThread.detach();
    //    }
    //    else if (identity == "SLAVE")
    //    {
    //        std::cout << "Slave connected" << std::endl;
    //        std::thread slaveThread(handle_slave, new_conn);
    //        slaveThread.detach();
    //        // slaves.push_back(new_conn);
    //    }

    //}
    while (true) {
        int new_conn = accept(server_fd, (struct sockaddr*)&address, &addrlen);

        if (new_conn == INVALID_SOCKET) {
            printf("accept failed with error: %d\n", WSAGetLastError());
            continue;
        }

        char buffer[1024] = { 0 };
        int result = recv(new_conn, buffer, sizeof(buffer) - 1, 0);

        if (result > 0) {
            buffer[result] = '\0'; // Null-terminate the received data
            std::string identity = buffer;

            if (identity == "CLIENT") {
                std::cout << "Client connected" << std::endl;
                std::thread clientThread(handle_client, new_conn);
                clientThread.detach();
            }
            else if (identity == "SLAVE") {
                std::cout << "Slave connected" << std::endl;
                std::thread slaveThread(handle_slave, new_conn);
                slaveThread.detach();
            }
        }
        else if (result == 0) {
            printf("Connection closing...\n");
            closesocket(new_conn);
        }
        else {
            printf("recv failed with error: %d\n", WSAGetLastError());
            closesocket(new_conn);
        }
    }

    
    int bytesReceived = recv(new_socket, buffer, 1024, 0);
    if (bytesReceived == SOCKET_ERROR) {
        printf("recv failed with error: %d\n", WSAGetLastError());
    }
    else {
        buffer[bytesReceived] = '\0'; // Null-terminate the buffer
        printf("Received message: %s\n", buffer);
    }
    send(new_socket, greeting, (int)strlen(greeting), 0);
    printf("Hello message sent\n");

    // Cleanup
    closesocket(new_socket);
    closesocket(server_fd);
    WSACleanup();

    return 0;
}