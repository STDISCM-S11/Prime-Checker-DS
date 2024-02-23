#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    char buffer[1024] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8888);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    std::string identity = "CLIENT";

    send(sock, identity.c_str(), identity.size(), 0);

    char msg[1024] = {0};

    while (true)
    {
        std::cout << "message:" << std::endl;
        std::cin >> msg;
        send(sock, msg, strlen(msg), 0);
    }
    // send(sock, hello, strlen(hello), 0);
    // printf("Hello message sent\n");
    // read(sock, buffer, 1024);
    // printf("%s\n", buffer);
    return 0;
}
