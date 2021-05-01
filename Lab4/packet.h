#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>

#define MAX_NAME 32
#define MAX_DATA 1024

#define BUFFERLENGTH 1500
typedef struct packet
{
    unsigned int type; // msgType
    unsigned int size; // Size of data
    unsigned char source[MAX_NAME];
    unsigned char data[MAX_DATA];
} Packet;

// translate buffer into packet
void decode(const char *buffer, Packet *packet)
{

    char *search = ":";
    char *data_type = strtok(buffer, search);
    packet->type = atoi(data_type);

    char *size = strtok(NULL, search);
    packet->size = atoi(size);

    char *source = strtok(NULL, search);
    strcpy(packet->source, source);

    char *data = strtok(NULL, "");
    if (data == NULL)
        return;

    strcpy(packet->data, data);

    return;
}

//the reverse of decode
void encode(const Packet *packet, char *buffer)
{

    memset(buffer, 0, BUFFERLENGTH);
    int pointer = sprintf(buffer, "%d:%d:%s:", packet->type, packet->size, packet->source);
    memcpy((&buffer[pointer]), packet->data, packet->size);
    // printf("packet: %s\n", buffer);
    // printf("data packet->source :%s\n", packet->source);
    // printf("data length %u\n", strlen(packet->data));
}

//list of request types
enum Rquest_Type
{
    LOGIN,
    LO_ACK,
    LO_NAK,
    EXIT,
    JOIN,
    JN_NAK,
    JN_ACK,
    LEAVE_SESS,
    NEW_SESS,
    MESSAGE,
    NS_ACK,
    QUERY,
    QU_ACK,
    INVITE
};