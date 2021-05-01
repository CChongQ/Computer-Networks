#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

// #define MAX_LEN 1000
// #define ACK "ACK"
// #define NACK "NACK"
// #define BUF_LEN 1024

#include "packet.h"

struct packet *  detach_packet(char* buf,struct packet* return_packet);

int main(int argc, char const *argv[]){

    if (argc != 2) { 
        printf("Invalid input format. Correct format: server <UDP listen port>\n");
        exit(1);
    }
    //get port number
    int port_num = atoi(argv[1]);

    //create sockets
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sockfd < 0) {
        printf("Error: Create socket failed\n");
        exit(1);
    }
    struct sockaddr_in server;  
    server.sin_family = AF_INET;
    server.sin_port= htons(port_num);
    server.sin_addr.s_addr = INADDR_ANY;
    memset(server.sin_zero, '\0', sizeof(server.sin_zero));

    //bind: assign address & port to socket
    if ( bind(sockfd,(struct sockaddr *)&server,sizeof(server)) < 0 ){
        printf("Error: Binding\n");
        exit(1);
    }

    //listen: receive connection request 
    struct sockaddr_in from;  
    int buf_size = 1024;
    char buf[buf_size];
    memset(buf, '\0', buf_size);
    socklen_t from_len = sizeof(from);
    int num_bytes = recvfrom(sockfd,buf,buf_size,0,(struct sockaddr *)&from,&from_len);
    if (num_bytes<0){
        printf("Error: recvfrom\n");
        exit(1);
    }

    //send message to client
    int num_bytes_sent;
    if ( strcmp(buf,"ftp") == 0 ){ 
        char msg[] = "yes";
        num_bytes_sent = sendto(sockfd,msg,strlen(msg),0,(struct sockaddr *)&from,sizeof(from));
    }
    else{
        char msg[] = "no";
        num_bytes_sent = sendto(sockfd,msg,strlen(msg),0,(struct sockaddr *)&from,sizeof(from));
    }
 
    if (num_bytes_sent<0){
        printf("Error: Reply message  to the client failed.\n");
        exit(1);
    }
    else{
        struct sockaddr_in from;  
        socklen_t from_len = sizeof(from);
    
        FILE * file_tb_write;

        while(1){
            //recevie
            char buf[BUF_LEN];
            memset(buf, '\0', buf_size);
            int num_bytes = recvfrom(sockfd,buf,buf_size,0,(struct sockaddr *)&from,&from_len);
            // printf("Server buf %s fragment\n",buf);
            if (num_bytes<0){
                printf("Server Error: recvfrom\n");
                char msg[] = NACK;
                num_bytes_sent = sendto(sockfd,msg,strlen(msg),0,(struct sockaddr *)&from,sizeof(from));
                if (num_bytes_sent<0){
                    printf("Error: Reply message to the client failed.\n");
                    exit(1);
                }
            }
            else{
                //detach data
                struct packet temp_packet;
                detach_packet(buf,&temp_packet);
                printf("Processing the %d fragment\n",temp_packet.frag_no );

                if (temp_packet.frag_no == 1){//first fragment

                    char c[] = "recv_";
                    strcat(c,temp_packet.filename);
                    printf(" Server side file name is %s \n", c );

                    file_tb_write = fopen(c, "wb");
                }

                fwrite(temp_packet.filedata, sizeof(char), temp_packet.size, file_tb_write);


                //send ack or nack
                char msg[] = ACK;
                num_bytes_sent = sendto(sockfd,msg,strlen(msg),0,(struct sockaddr *)&from,sizeof(from));
                if (num_bytes_sent<0){
                    printf("Error: Reply message to the client failed.\n");
                    exit(1);
                }
                if (temp_packet.frag_no == temp_packet.total_frag){
                    printf("Processed ALL fragment\n");
                    break;
                }
                
            }
        }
        fclose(file_tb_write);
       
    }

    // freeaddrinfo(server);
    close(sockfd);
    return (0);

}

struct packet *  detach_packet(char* buf,struct packet* return_packet){

    unsigned int total_frag,frag_no,size;
    char* filename;

    total_frag = atoi(strtok(buf, ":"));
    printf("Server total_frag %d \n",total_frag);
    frag_no = atoi(strtok(NULL, ":"));
    printf("Server frag_no %d \n",frag_no);
    size = atoi(strtok(NULL, ":"));
    printf("Server size %d \n",size);
    filename = strtok(NULL, ":");
    printf("Server filename %s \n",filename);

    char * fileData = strtok(NULL, "");
    // printf("Server fileData %s \n",fileData);

    // struct packet* return_packet;
    // return_packet = malloc(size*sizeof(char));
    return_packet->total_frag = total_frag;
    return_packet->frag_no = frag_no;
    return_packet->size = size;
    return_packet->filename = filename;
    memcpy(return_packet->filedata, fileData, return_packet->size );

    return return_packet;


}