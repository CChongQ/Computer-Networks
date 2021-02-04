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


int main(int argc, char const *argv[]){
    
    if (argc != 3) { 
        printf("Invalid input format. Correct format: deliver <server address> <server port number>\n");
        exit(1);
    }

    //get host name +  port number
    const char *hostname = argv[1];
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
    server.sin_port= htons(port_num);
    //get ip using host name
    struct hostent *he = gethostbyname(hostname) ;
	struct in_addr **addr_list;
    if ( he == NULL) {
		printf("Error: Unknown host\n");
		exit(1);
	}
    addr_list = (struct in_addr **) he->h_addr_list;
    server.sin_addr = *addr_list[0];

    //require user input
    int max_input= 1024;
    char userInput[max_input];
    printf("Please input file name using this format: ftp <file name>\n");
    fgets(userInput,max_input, stdin);

    //get filename
    char *search = " ";
    char *token_1 = strtok(userInput, search);
    char *fileName= strtok(NULL, search);
    if (strcmp(token_1,"ftp")){
        printf("Error: Input must begin with ftp\n");
        exit(1);
    }
    if (*fileName == 0){
         printf("Error: filename cannot be empty");
    }
    //trim filename
    char *end;
    end = fileName + strlen(fileName) - 1;
    while(end > fileName && isspace((unsigned char)*end)) end--;
    end[1] = '\0';

    if (access(fileName, F_OK) != 0 ){
        fprintf(stderr,"Error: File < %s > does not exists\n",fileName);
        exit(1);
    }
    else{
        //send a message Â“ftpÂ” to the server
        int num_bytes = sendto(sockfd,"ftp",strlen("ftp"),0,(struct sockaddr *)&server,sizeof(server));
        if (num_bytes<0){
            printf("Error: Send message 'ftp' to the server failed.\n");
            exit(1);
        }
        //Receive a message from the server:
        struct sockaddr_in from;  
        int buf_size = 1024;
        char buf[buf_size];
        socklen_t from_len = sizeof(from);
        int num_bytes_from = recvfrom(sockfd,buf,buf_size,0,(struct sockaddr *)&from,&from_len);
        if (num_bytes_from<0){
            printf("Error: recvfrom\n");
            exit(1);
        }

        if (strcmp(buf,"yes") == 0){
             printf("A file transfer can start.\n");
        }
        else{
            printf("Error: Received message is not 'yes'\n");
            exit(1);
        }
    }

    close(sockfd);
    return (0);

}
