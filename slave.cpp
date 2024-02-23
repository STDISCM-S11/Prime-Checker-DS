#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <vector>
#include <thread>

// using namespace std;

int main()
{
    sockaddr_in master_server_addr;
    int opt = 1;
    int addr_len = sizeof(master_server_addr);
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    master_server_addr.sin_family = AF_INET;
    master_server_addr.sin_addr.s_addr = INADDR_ANY;
    master_server_addr.sin_port = htons(8888);

    if (inet_pton(AF_INET, "127.0.0.1", &master_server_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(server_sock, (struct sockaddr *)&master_server_addr, sizeof(master_server_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    std::string identity = "SLAVE";

    send(server_sock, identity.c_str(), identity.size(), 0);

    char buffer[1024] = {0};
    char *slave_message = "send to master";
    while (true)
    {
        read(server_sock, buffer, sizeof(buffer));
        if (strcmp(buffer, "hello") == 0)
        {
            std::cout << "Message from master: " << buffer << std::endl;
            send(server_sock, slave_message, strlen(slave_message), 0);
        }
    }

    return 0;
    // int binding = bind(server_sock, &address, addr_len < 0);
}