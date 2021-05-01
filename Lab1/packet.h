#define MAX_LEN 1000
#define ACK "ACK"
#define NACK "NACK"
#define BUF_LEN 1024

struct packet {
    unsigned int total_frag; // total number of fragments
    unsigned int frag_no; //sequence number of the fragment
    unsigned int size; //size of the data
    char* filename;
    char filedata[1000]; //single string
};  