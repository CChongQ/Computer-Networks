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

    close(sockfd);
    return (0);

}