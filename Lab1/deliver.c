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
    
    if (argc != 3) { 
        printf("Invalid input format. Correct format: deliver <server address> <server port number>\n");
        exit(1);
    }

    //get host address +  port number
    int host = argv[1];
    int port_num = atoi(argv[2]);

    //create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(sockfd < 0) {
        printf("Error: Create socket failed\n");
        exit(1);
    }

    struct sockaddr_in server;  
    memset(&server, 0, sizeof server);
    server.sin_family = AF_INET;
    int hp = gethostbyname(host);
    if (hp==0){
        printf("Error: Unknown host\n");
        exit(1);
    }
    server.sin_port= htons(port_num);

    //require user input
    int max_input= 1024;
    char userInput[max_input];
    printf("Please input input name using this format: ftp <file name>\n");
    fgets(userInput,max_input, stdin);
    //get filename
    char *search = " ";
    char *token_1 = strtok(userInput, search);
    char *fileName= strtok(NULL, search);
    if (strcmp(token_1,"ftp")){
        printf("Error: Input must begin with ftp\n");
        exit(1);
    }
    else if (access(fileName, F_OK) != 0 ){
        printf("Error: File < %c > does not exists\n",fileName);
        exit(1);
    }
    else{
        //send a message “ftp” to the server
        int num_bytes = sendto(sockfd,"ftp",strlen("ftp"),0(struct sockaddr *)&server,sizeof(server));
        if (num_bytes<0){
            printf("Error: Send message 'ftp' to the server failed.\n");
            exit(1);
        }
        //Receive a message from the server:
        struct sockaddr_in from;  
        int buf_size = 1024;
        char buf[buf_size];
        int num_bytes_from = recvfrom(sockfd,buf,buf_size,0,(struct sockaddr *)&from,sizeof(from));
        if (num_bytes_from<0){
            printf("Error: recvfrom\n");
            exit(1);
        }

        if (strcmp(buf,"yes") == 0){
             printf("Error: A file transfer can start.\n");
        }
        else{
            printf("Error: Received message is not 'yes'\n");
            exit(1);
        }
    }

    close(sockfd);
    return (0);

}