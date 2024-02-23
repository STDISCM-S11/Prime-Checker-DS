#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <thread>

void handle_slave(int socket)
{
    char buffer[1024] = {0};
    char *message = "hello";
    send(socket, message, strlen(message), 0);
    std::cout << "Message sent to slave server\n";
    read(socket, buffer, 1024); // Example of reading a message from slave
    std::cout << "Message from slave: " << buffer << std::endl;
}

void handle_client(int socket)
{
    char buffer[1024] = {0};
    while (true)
    {
        int val = recv(socket, buffer, sizeof(buffer), 0);
        if (val <= 0)
        {
            std::cerr << "Error" << std::endl;
            close(socket);
            break;
        }

        std::cout << buffer << std::endl;
    }
}

int main()
{
    sockaddr_in address;
    int opt = 1;
    int addr_len = sizeof(address);
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    if (server_sock == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8888);

    if (bind(server_sock, (sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_sock, 3) < 0)
    {
        perror("listen");

        exit(EXIT_FAILURE);
    }

    std::vector<int> slaves;

    while (true)
    {
        int new_conn = accept(server_sock, (sockaddr *)&address, (socklen_t *)&addr_len);
        if (new_conn < 0)
        {
            perror("error slave connection");
            continue;
        }

        char buffer[1024] = {0};
        recv(new_conn, buffer, sizeof(buffer), 0);

        std::string identity = buffer;

        if (identity == "CLIENT")
        {
            std::cout << "Client connected" << std::endl;
            std::thread clientThread(handle_client, new_conn);
            clientThread.detach();
        }
        else if (identity == "SLAVE")
        {
            std::cout << "Slave connected" << std::endl;
            std::thread slaveThread(handle_slave, new_conn);
            slaveThread.detach();
            // slaves.push_back(new_conn);
        }

        // std::cout << "message from client " << buffer << std::endl;
    }

    return 0;
    // int binding = bind(server_sock, &address, addr_len < 0);
}