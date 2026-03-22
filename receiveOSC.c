//
// Created by Alex Kelly on 3/22/26.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


int create_and_bind(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket creation failed");
        exit(1);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind failed");
        close(sock);
        exit(1);
    }

    return sock;
}

void receive_loop(int sock)
{
    char buffer[1024];

    while (1)
    {
        struct sockaddr_in sender;
        socklen_t senderlen = sizeof(sender);

        int n = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
            (struct sockaddr*)&sender, &senderlen);

        if (n < 0)
        {
            perror("recvfrom failed");
            continue;
        }

        buffer[n] = '\0';
        printf("Recieved: %s\n", buffer);
    }
}

int main()
{
    int sock = create_and_bind(9004);

    receive_loop(sock);

    close(sock);
    return 0;

}