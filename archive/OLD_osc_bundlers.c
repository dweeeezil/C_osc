//
// Created by Alex Kelly on 3/22/26.
//



int OLD_osc_bundle_add_float32(OscBundle *b, const char *address, float value)
{
    char msg[256] = {0}; //allocates clean memory to write to
    int offset = 0; //sets beginning

    offset = write_padded_string(msg, offset, address); //writes address to chunk, moves offset to beginning
    offset = write_padded_string(msg, offset, ",f");
    offset = write_float32(msg, offset, value);

    if (b->offset + 4 + offset > b->capacity)
    {
        fprintf(stderr, "OSC bundle overflow\n");
        return -1;
    }

    int32_t be_size = htonl(offset);  //calcs size of everything written in msg
    memcpy(b->buffer + b->offset, &be_size, 4);
    b->offset += 4;

    memcpy(b->buffer + b->offset, msg, offset); //copy to buffer
    b->offset += offset;

    return 0;
}
int OLD_osc_bundle_add_int32(OscBundle *b, const char *address, int value)
{
    char msg[256] = {0};
    int offset = 0;

    offset = write_padded_string(msg, offset, address);
    offset = write_padded_string(msg, offset, ",i");
    offset = write_int32(msg, offset, value);

    if (b->offset + 4 + offset > b->capacity)
    {
        fprintf(stderr, "OSC bundle overflow\n");
        return -1;
    }

    int32_t be_size = htonl(offset);
    memcpy(b->buffer + b->offset, &be_size, 4);
    b->offset += 4;

    memcpy(b->buffer + b->offset, msg, offset);
    b->offset += offset;

    return 0;
}
int OLD_osc_bundle_add_string(OscBundle *b, const char *address, const char *str)
{
    char msg[256] = {0};
    int offset = 0;

    offset = write_padded_string(msg, offset, address);
    offset = write_padded_string(msg, offset, ",s");
    offset = write_padded_string(msg, offset, str);

    if (b->offset + 4 + offset > b->capacity)
    {
        fprintf(stderr, "OSC bundle overflow\n");
        return -1;
    }

    int32_t be_size = htonl(offset);
    memcpy(b->buffer + b->offset, &be_size, 4);
    b->offset += 4;

    memcpy(b->buffer + b->offset, msg, offset);
    b->offset += offset;

    return 0;
}
