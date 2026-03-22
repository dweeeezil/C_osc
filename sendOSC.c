#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdarg.h>
#include <stdint.h>

int pad4(int n) //rounds up to next multiple of 4
{
    return (n + 3) & ~0x03;
}

int write_padded_string(char *buf, int offset, const char *str)
{
    int len = strlen(str) + 1; //include null terminator
    int padded = pad4(len);

    memcpy(buf + offset, str, len); //zero padding already assumed

    return offset + padded;
}


// write int32 (big endian)
int write_int32(char *buf, int offset, int32_t val)
{
    int32_t be = htonl(val);
    memcpy(buf + offset, &be, 4); //copies &be to buf + offset address with a length of 4 bytes
    return offset + 4;
}

//write float32 (big endian)
int write_float32(char *buf, int offset, float val)
{
    uint32_t temp;
    memcpy(&temp, &val, 4);
    temp = htonl(temp);
    memcpy(buf + offset, &temp, 4);
    return offset + 4;
}


int create_socket()
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0); //SOCK_STREAM for TCP, SOCK_DGRAM for UDP
    if (sock < 0)
    {
        perror("Socket creation failed");
    }
    return sock;
}

void connect_socket(int sock, const char *ip, int port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("Connection failed");
        close(sock);
        exit(1);
    }
}



void send_osc(int sock,
                const char *address,
                const char *types, ...)
{
    char buffer[1024] = {0};
    int offset = 0;

    // 1. Address
    offset = write_padded_string(buffer, offset, address);

    // 2. Type tag string (must start with ',')
    char type_tag[64] = ",";
    strcat(type_tag, types);
    offset = write_padded_string(buffer, offset, type_tag);

    // 3. Arguments
    va_list args;
    va_start(args, types);

    for (int i = 0; types[i] != '\0'; i++)
        {
        switch (types[i]) {
            case 'i': {
                int val = va_arg(args, int);
                offset = write_int32(buffer, offset, val);
                break;
            }
            case 'f': {
                double val = va_arg(args, double); // float promoted to double
                offset = write_float32(buffer, offset, (float)val);
                break;
            }
            case 's': {
                char *str = va_arg(args, char *);
                offset = write_padded_string(buffer, offset, str);
                break;
            }
            default:
                fprintf(stderr, "Unsupported OSC type: %c\n", types[i]);
                break;
        }
    }

    va_end(args);

    int bytes = send(sock, buffer, offset, 0);
    if (bytes < 0)
    {
        perror("send failed");
    }

}


int main()
{
    int sock = create_socket();
    connect_socket(sock, "127.0.0.1", 9008);


    float test = 0.0f;

    while (1)
    {
        send_osc(sock, "/fader1", "f", test);
        test += 0.1f;
        usleep(100000); //100ms
    }
    close(sock);
    return 0;
}