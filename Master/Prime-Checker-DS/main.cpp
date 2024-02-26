#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <vector>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <string>

#pragma comment (lib, "Ws2_32.lib")

std::vector<int> slave_sockets; // List of slave sockets
std::mutex slave_sockets_mutex; // Mutex for protecting the slave sockets list
std::unordered_map<int, int> client_requests; // Maps a request ID to a client socket

void handle_slave(int slave_socket) {
    char buffer[1024] = { 0 };
    // Assume the slave is always ready to process messages after connecting
    while (true) {
        int bytesReceived = recv(slave_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            // Handle errors or closure
            break;
        }
        buffer[bytesReceived] = '\0';

        // Process the message and determine which client it is for
        // Assuming the message format is "request_id:message"
        std::lock_guard<std::mutex> lock(slave_sockets_mutex);
        if (!slave_sockets.empty()) {
            // Send the response back to the client
            send(slave_sockets[0], buffer, bytesReceived, 0);
        }
        // Echo the message back to the master for now
        //send(slave_socket, buffer, bytesReceived, 0);
    }
}

void handle_client(int client_socket) {
    char buffer[1024] = { 0 };
    while (true) {
        int bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            // Handle errors or closure
            break;
        }
        // Assign a unique request ID for each client request
        static int request_id = 0; // This should be made thread-safe in a real-world scenario
        std::string request = std::to_string(request_id++) + ":" + std::string(buffer, bytesReceived);

        // Forward the request to a slave
        std::lock_guard<std::mutex> lock(slave_sockets_mutex);
        if (!slave_sockets.empty()) {
            send(slave_sockets[0], buffer, bytesReceived, 0);
            // Do not wait for the response here, handle_slave will handle it
        }
        else {
            std::cerr << "No slaves connected.\n";
            // Send an error message back to the client or handle accordingly
            const char* errMsg = "No slaves available.";
            send(client_socket, errMsg, strlen(errMsg), 0);
        }
        // Forward the message to the first slave
        //{
        //    std::lock_guard<std::mutex> lock(slave_sockets_mutex);
        //    if (!slave_sockets.empty()) {
        //        send(slave_sockets[0], buffer, bytesReceived, 0);
        //        // Wait for the response from the slave
        //        int bytesFromSlave = recv(slave_sockets[0], buffer, sizeof(buffer), 0);
        //        if (bytesFromSlave > 0) {
        //            // Send the response from the slave back to the client
        //            send(client_socket, buffer, bytesFromSlave, 0);
        //        }
        //        else {
        //            // Handle errors or closure
        //            break;
        //        }
        //    }
        //    else {
        //        std::cerr << "No slaves connected.\n";
        //        // Send an error message back to the client or handle accordingly
        //        const char* errMsg = "No slaves available.";
        //        send(client_socket, errMsg, strlen(errMsg), 0);
        //    }
        //}
    }
    closesocket(client_socket); // Close the client socket when done
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
				slave_sockets.push_back(new_conn);
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