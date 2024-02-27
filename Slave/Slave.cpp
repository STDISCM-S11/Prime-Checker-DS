#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <mutex>
#include <thread>
#include <vector>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")

bool check_prime(const int& num) {
    for (int i = 2; i * i <= num; i++) {
        if (num % i == 0) {
            return false;
        }
    }
    return true;
}

void primesCheck(int start, int end, std::vector<int>& primes,
    std::mutex& mutex) {
    for (int current_num = start; current_num <= end; current_num++) {
        if (check_prime(current_num)) {
            std::unique_lock<std::mutex> lock(mutex);
            primes.push_back(current_num);
            lock.unlock();
            //std::cout << current_num << " is prime.\n"; // Debug output
        }
        //else {
        //    std::cout << current_num << " is not prime.\n"; // Debug output
        //}
    }
}

int primeCheckerMain(int reqStart, int reqEnd) {
    int totalRange = reqEnd - reqStart + 1;
    int threadCount = min(1, totalRange); // Adjust thread count based on range

    std::vector<int> primes;
    std::vector<std::thread> threads;
    std::mutex mutex;

    int baseRange = totalRange / threadCount;
    int remain = totalRange % threadCount;

    for (int i = 0; i < threadCount; i++) {
        int start = reqStart + i * baseRange;
        int end = (i < threadCount - 1) ? start + baseRange - 1 : reqEnd;

        threads.push_back(std::thread(primesCheck, start, end, std::ref(primes), std::ref(mutex)));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    std::cout << primes.size() << " primes were found." << std::endl;

    return primes.size();
}

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
            
            char* ptr; // declare a ptr pointer
            char* nextToken = nullptr;
            char* context = nullptr;
            rsize_t strmax = sizeof(buffer); // define the maximum size of the string
            const char* delimiters = ","; // define the delimiters
            std::vector<int> request;

            ptr = strtok_s(buffer, delimiters, &context);
            while (ptr != NULL) {
                std::string val = ptr; // Convert the token to a string
                request.push_back(std::stoi(val)); // Convert and store the integer value
                ptr = strtok_s(NULL, delimiters, &context); // Continue to tokenize the string
            }

            // Construct the response message
            std::string response_message = std::to_string(primeCheckerMain(request[0], request[1]));

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
