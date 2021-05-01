#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>


#define MAX_LEN 1000
#define MAX_PACKET_NUM 5000
#define ACK "ACK"
#define NACK "NACK"
#define BUF_LEN 1024
#define ALPHA 0.125
#define BETA 0.25

struct packet {
    unsigned int total_frag; // total number of fragments
    unsigned int frag_no; //sequence number of the fragment
    unsigned int size; //size of the data
    char* filename;
    char filedata[1000]; //single string
};  

// struct timeval {
//  int tv_sec; // the number of seconds to wait,
//  int tv_usec; // the number of microseconds to wait
// };