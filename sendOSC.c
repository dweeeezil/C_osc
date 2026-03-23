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




typedef struct
{
    char *buffer;
    int capacity;
    int offset;
} OscBundle;

void osc_bundle_init(OscBundle *b, char *buffer, int capacity);
int osc_bundle_add(OscBundle *b, const char *address, const char *types, ...);
int osc_bundle_add_float32(OscBundle *b, const char *address, float value);
int osc_bundle_add_int32(OscBundle *b, const char *address, int value);
int osc_bundle_add_string(OscBundle *b, const char *address, const char *str);
int osc_bundle_send(OscBundle *b, int sock);


void osc_bundle_init(OscBundle *b, char *buffer, int capacity)
{
    b->buffer = buffer;
    b->capacity = capacity;
    b->offset = 0;

    // "#bundle" + padding (8 bytes total)
    char bStr[7] = "#bundle";
    memcpy(b->buffer + b->offset, bStr, strlen(bStr) - 1);
    b->offset += strlen(bStr);



    //timetag (8bytes) immediate
    memset(b->buffer + b->offset, 0, 8);
    b->offset += 8;
}

int osc_bundle_add(OscBundle *b, const char *address, const char *types, ...)
{
    char msg[512] = {0};
    int msg_offset = 0;

    // build osc message
    msg_offset = write_padded_string(msg, msg_offset, address);

    char type_tag[64];
    snprintf(type_tag, sizeof(type_tag), ",%s", types);
    msg_offset = write_padded_string(msg, msg_offset, type_tag);

    va_list args;
    va_start(args, types);

    for (int i = 0; types[i] != '\0'; i++)
    {
        switch (types[i])
        {
            case 'i':
            {
                int val = va_arg(args, int);
                msg_offset = write_int32(msg, msg_offset, val);
                break;
            }
                case 'f':
            {
                double val = va_arg(args, double);
                msg_offset = write_float32(msg, msg_offset, (float)val);
                break;
            }
                case 's':
            {
                char *str = va_arg(args, char *);
                msg_offset = write_padded_string(msg, msg_offset, str);
                break;
            }
            default:
                fprintf(stderr, "Unsupported OSC type: %c\n", types[i]);
                va_end(args);
                return -1;
        }
    }

    va_end(args);

    // --- bounds check ---
    if (b->offset + 4 + msg_offset > b->capacity) {
        fprintf(stderr, "OSC bundle overflow\n");
        return -1;
    }

    // --- write size prefix ---
    int32_t be_size = htonl(msg_offset);
    memcpy(b->buffer + b->offset, &be_size, 4);
    b->offset += 4;

    // --- write message ---
    memcpy(b->buffer + b->offset, msg, msg_offset);
    b->offset += msg_offset;

    return 0;
}


int osc_bundle_add_float32(OscBundle *b, const char *address, float value) //New and improved
{
    int _offset = b->offset + 4; //sets beginning NEW - Writes directly to buffer at latest offset


    _offset = write_padded_string(b->buffer, _offset, address);
    _offset = write_padded_string(b->buffer, _offset, ",f");
    _offset = write_float32(b->buffer, _offset, value);

    int32_t be_size = htonl(_offset - (b->offset + 4));  //calcs size of everything written in msg

    if (_offset > b->capacity)
    {
        fprintf(stderr, "OSC bundle overflow\n");
        return -1;
    } //Error Checking

    memcpy(b->buffer + b->offset, &be_size, 4);

    b->offset = _offset;

    return 0;
}

int osc_bundle_add_int32(OscBundle *b, const char *address, int value)
{
    int _offset = b->offset + 4;

    _offset = write_padded_string(b->buffer, _offset, address);
    _offset = write_padded_string(b->buffer, _offset, ",i");
    _offset = write_int32(b->buffer, _offset, value);

    int32_t be_size = htonl(_offset - (b->offset + 4));

    if (_offset > b->capacity)
    {
        fprintf(stderr, "OSC bundle overflow\n");
        return -1;
    } //Error Checking

    memcpy(b->buffer + b->offset, &be_size, 4);

    b->offset = _offset;

    return 0;
}

int osc_bundle_add_string(OscBundle *b, const char *address, const char *str)
{
    int _offset = b->offset + 4;

    _offset = write_padded_string(b->buffer, _offset, address);
    _offset = write_padded_string(b->buffer, _offset, ",s");
    _offset = write_padded_string(b->buffer, _offset, str);

    int32_t be_size = htonl(_offset - (b->offset + 4));

    if (_offset > b->capacity)
    {
        fprintf(stderr, "OSC bundle overflow\n");
        return -1;
    } //Error Checking

    memcpy(b->buffer + b->offset, &be_size, 4);

    b->offset = _offset;

    return 0;
}


int osc_bundle_send(OscBundle *b, int sock)
{
    int bytes = send(sock, b->buffer, b->offset, 0);
    if (bytes < 0) {
        perror("send");
        return -1;
    }
    return bytes;
}







int main()
{
    int sock = create_socket();
    connect_socket(sock, "127.0.0.1", 9008);

    char bundle_buffer[2048];

    float t = 0.0f;
    int i = 0;

    while (1)
    {
        OscBundle bundle;
        osc_bundle_init(&bundle, bundle_buffer, sizeof(bundle_buffer));

        osc_bundle_add_float32(&bundle, "/test/float", t);
        osc_bundle_add_int32(&bundle, "/test/int", i);
        osc_bundle_add_string(&bundle, "/test/string", "hello world");

        //osc_bundle_add(&bundle, "/fader1", "f", t);
        //osc_bundle_add(&bundle, "/fader2", "f", t * 0.2f);
        //osc_bundle_add(&bundle, "/xy", "ff", t, 1.0f - t);


        osc_bundle_send(&bundle, sock);

        t += 0.5f;
        i += 1;
        usleep(16000); //100ms
    }
    close(sock);
    return 0;
}