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
#include <future>

#pragma comment (lib, "Ws2_32.lib")

std::vector<int> slave_sockets; // List of slave sockets
int client_socket;
std::mutex slave_sockets_mutex; // Mutex for protecting the slave sockets list
std::unordered_map<int, int> client_requests; // Maps a request ID to a client socket
std::vector<int> request;
std::vector<int> masterTask;
std::vector<int> slaveTask;
int numPrimes = 0;

std::atomic<int> total_prime_count{ 0 };
std::future<int> master_future; // Future to get result from the master
std::promise<int> slave_prime_promise; // Promise to hold the slave's result
std::future<int> slave_prime_future = slave_prime_promise.get_future(); // Future to wait on the slave's result
std::mutex mutex;



bool check_prime(const int& num) {
    if (num < 2) return false; // Ensure we handle numbers less than 2
    for (int i = 2; i * i <= num; i++) {
        if (num % i == 0) {
            return false;
        }
    }
    return true;
}

void primesCheck(int start, int end,/* std::promise<int>& primePromise*/ std::vector<int>& primes, std::mutex& mutex) {
    //std::vector<int> primes;
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
    //primePromise.set_value(primes.size());
}

int primeCheckerMain(int reqStart, int reqEnd, int *numPrimes) {
    int totalRange = reqEnd - reqStart + 1;
    int threadCount = min(32, totalRange); // Adjust thread count based on range

    std::vector<int> primes;
    //std::vector<std::future<int>> futures;
    std::vector<std::thread> threads;
    std::mutex mutex;

    int baseRange = totalRange / threadCount;
    int remain = totalRange % threadCount;

    
    /*for (int i = 0; i < threadCount; i++) {
        std::promise<int> primePromise;
        futures.push_back(primePromise.get_future());
        int start = reqStart + i * baseRange;
        int end = (i < threadCount - 1) ? start + baseRange - 1 : reqEnd;
        std::thread(primesCheck, start, end, std::ref(primePromise), std::ref(mutex)).detach();
    }*/
    for (int i = 0; i < threadCount; i++) {
        int start = reqStart + i * baseRange;
        int end = (i < threadCount - 1) ? start + baseRange - 1 : reqEnd;

        threads.push_back(std::thread(primesCheck, start, end, std::ref(primes), std::ref(mutex)));
    }
    for (auto& thread : threads) {
        thread.join();
    }
    *numPrimes += primes.size();
    
    std::cout << primes.size() << " primes were found." << std::endl;

    return primes.size();

    //for (auto& future : futures) {
    //    total_prime_count += future.get(); // This will wait for the thread to complete
    //}

    //std::cout << total_prime_count << " primes were found by master." << std::endl;

    //// Return the count
    //return total_prime_count;
}

void handle_slave(int slave_socket, int *numPrimes) {
    //char buffer[1024] = { 0 };
    //int bytesReceived = recv(slave_socket, buffer, sizeof(buffer) - 1, 0);
    //if (bytesReceived > 0) {
    //    buffer[bytesReceived] = '\0';
    //    int slave_prime_count = std::stoi(buffer);
    //    slave_prime_promise.set_value(slave_prime_count); // Set the slave's result
    //}
    //else {
    //    // Handle errors or closure
    //    slave_prime_promise.set_value(0); // Set to 0 if no primes or an error occurred
    //}
    //closesocket(slave_socket);
    char buffer[1024] = { 0 };
    // Assume the slave is always ready to process messages after connecting
    while (true) {
        int bytesReceived = recv(slave_socket, buffer, sizeof(buffer) - 1, 0);
        //if (bytesReceived <= 0) {
        //    // Handle errors or closure
        //    break;
        //}
        //buffer[bytesReceived] = '\0';

        //// Parse the received data to get the number of primes found by the slave
        //int slave_prime_count = std::stoi(buffer);
        //int master_prime_count = master_future.get(); // Wait for master to finish and get its count
        //int total_prime_count = master_prime_count + slave_prime_count;

        //// Now send the total prime count back to the client
        //std::string total_primes_str = std::to_string(total_prime_count);
        //send(client_socket, total_primes_str.c_str(), total_primes_str.length(), 0);
        if (bytesReceived <= 0) {
            // Handle errors or closure
            break;
        }
        buffer[bytesReceived] = '\0';
        *numPrimes += atoi(buffer);

        //send(client_socket, buffer, bytesReceived, 0); // client_socket needs to be tracked when the client connects
        break;
    }
    closesocket(slave_socket);
}

void handle_client(int client_socket) {
    char buffer[1024] = { 0 };
    while (true) {
        int bytesReceived = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytesReceived <= 0) {
            // Handle errors or closure
            break;
        }
        char* ptr; // declare a ptr pointer
        char* nextToken = nullptr;
        char* context = nullptr;
        rsize_t strmax = sizeof(buffer); // define the maximum size of the string
        const char* delimiters = ","; // define the delimiters

        ptr = strtok_s(buffer, delimiters, &context);
        while (ptr != NULL) {
            std::string val = ptr; // Convert the token to a string
            request.push_back(std::stoi(val)); // Convert and store the integer value
            ptr = strtok_s(NULL, delimiters, &context); // Continue to tokenize the string
        }

        // Forward the request to a slave
        // todo: add logic for many slaves
        // endpoint - start point div 2
        // end point is divide it by 2 || + 1
        // if 1000 master ends 500 slave starts 501
        // end point niya is current endpoint
        masterTask.push_back(request[0]);
        masterTask.push_back(ceil((request[1] - request[0]) / 2));

        std::cout << std::endl << masterTask[0] << " " << masterTask[1] << std::endl;

        /*std::promise<int> master_promise;
        master_future = master_promise.get_future();
        std::thread masterThread([reqStart = masterTask[0], reqEnd = masterTask[1], &master_promise]() {
            master_promise.set_value(primeCheckerMain(reqStart, reqEnd));
            });*/
        std::promise<int> master_promise;
        master_future = master_promise.get_future();
        std::thread masterThread(primeCheckerMain, masterTask[0], masterTask[1], &numPrimes);

        slaveTask.push_back(masterTask[1] + 1);
        slaveTask.push_back(request[1]);

        std::string msg;
        msg += std::to_string(slaveTask[0]);
        msg += ",";
        msg += std::to_string(slaveTask[1]);

        std::lock_guard<std::mutex> lock(slave_sockets_mutex);
        if (!slave_sockets.empty()) {
            send(slave_sockets[0], msg.c_str(), msg.size(), 0);
            // Do not wait for the response here, handle_slave will handle it
        }
        else {
            std::cerr << "No slaves connected.\n";
            // Send an error message back to the client or handle accordingly
            const char* errMsg = "No slaves available.";
            send(client_socket, errMsg, strlen(errMsg), 0);
        }        
        
        masterThread.join(); // Wait for the master to finish
        //int master_prime_count = master_future.get(); // Get the master's result
        //int slave_prime_count = slave_prime_future.get(); // Get the slave's result

        //total_prime_count = master_prime_count + slave_prime_count;
        //std::string total_primes_str = std::to_string(total_prime_count);
        //send(client_socket, total_primes_str.c_str(), total_primes_str.length(), 0); // Send the total count to the client
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
                client_socket = new_conn;
                clientThread.detach();
            }
            else if (identity == "SLAVE") {
                std::cout << "Slave connected" << std::endl;
                std::thread slaveThread(handle_slave, new_conn, &numPrimes);
				slave_sockets.push_back(new_conn);
                slaveThread.join();
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