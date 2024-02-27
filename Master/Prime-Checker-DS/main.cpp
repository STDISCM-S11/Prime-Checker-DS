#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <atomic>

#pragma comment(lib, "Ws2_32.lib")

std::vector<int> slave_sockets; // List of slave sockets
std::atomic<int> numPrimes{ 0 }; // Atomic to safely increment from multiple threads
std::mutex slave_sockets_mutex; // Mutex for protecting the slave sockets list

bool check_prime(const int& num) {
    if (num < 2) return false;
    for (int i = 2; i * i <= num; ++i) {
        if (num % i == 0) return false;
    }
    return true;
}

void primesCheck(int start, int end, std::atomic<int>& primesCount) {
    for (int current = start; current <= end; ++current) {
        if (check_prime(current)) {
            ++primesCount;
        }
    }

    std::cout << primesCount << " primes." << std::endl;
}

void handle_slave(int slave_socket, std::atomic<int>& primesCount) {
    char buffer[1024] = { 0 };
    int bytesReceived = recv(slave_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytesReceived > 0) {
        buffer[bytesReceived] = '\0';
        primesCount += atoi(buffer); // Convert buffer to int and add to total count
    }
    else {
        std::cerr << "Error receiving data from slave or connection closed\n";
    }
    closesocket(slave_socket);
}

void handle_client(int client_socket, std::atomic<int>& primesCount) {
    char buffer[1024] = { 0 };
    int bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytesReceived <= 0) {
        std::cerr << "Error receiving data from client or connection closed\n";
        return;
    }

    // Parse received range
    int start, end;
    sscanf_s(buffer, "%d,%d", &start, &end);

    // Calculate the midpoint of the range
    int mid = start + (end - start) / 2;

    // Start thread for master range
    std::thread masterThread(primesCheck, start, mid, std::ref(primesCount));

    // Prepare message for slave
    std::string message = std::to_string(mid + 1) + "," + std::to_string(end);
    std::lock_guard<std::mutex> lock(slave_sockets_mutex);
    if (!slave_sockets.empty()) {
        // Send range to the first available slave
        send(slave_sockets[0], message.c_str(), message.size(), 0);
        // Handle slave response in a separate thread
        std::thread slaveThread(handle_slave, slave_sockets[0], std::ref(primesCount));
        slaveThread.join(); // Wait for the thread to finish to get the result from the slave
    }
    else {
        std::cerr << "No slaves connected.\n";
        // If no slaves, master calculates entire range
        primesCheck(mid + 1, end, primesCount);
    }

    // Wait for the master thread to finish its calculations
    masterThread.join();

    // Send the total prime count back to the client
    std::string total_primes_str = std::to_string(primesCount.load());
    send(client_socket, total_primes_str.c_str(), total_primes_str.size(), 0);

    // Close client socket after sending data
    closesocket(client_socket);
}

int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return 1;
    }

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Socket creation failed with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    std::cout << "Server is running and waiting for connections...\n";

    while (true) {
        struct sockaddr_in client_address;
        int addrlen = sizeof(client_address);
        SOCKET new_conn = accept(server_fd, (struct sockaddr*)&client_address, &addrlen);

        if (new_conn == INVALID_SOCKET) {
            std::cerr << "Accept failed with error: " << WSAGetLastError() << std::endl;
            continue;
        }

        char identity[1024] = { 0 };
        recv(new_conn, identity, sizeof(identity) - 1, 0);
        if (strcmp(identity, "CLIENT") == 0) {
            std::cout << "Client connected\n";
            std::thread clientThread(handle_client, new_conn, std::ref(numPrimes));
            clientThread.detach(); // Detach the thread because we do not need to join it later
        }
        else if (strcmp(identity, "SLAVE") == 0) {
            std::cout << "Slave connected\n";
            std::lock_guard<std::mutex> guard(slave_sockets_mutex);
            slave_sockets.push_back(new_conn);
        }
        else {
            std::cerr << "Unrecognized identity\n";
            closesocket(new_conn);
        }
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}