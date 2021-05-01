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

#include "packet.h"

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
    int max_input= BUF_LEN;
    char userInput[max_input]; 
    printf("Please input file name using this format: ftp <file name>\n");
    fgets(userInput,max_input, stdin);

    //get filename
    char *search = " ";
    char *token_1 = strtok(userInput, search);
    char *fileName= strtok(NULL, search);
    if (strcmp(token_1,"ftp")){ //bug to fix: enter ftp(only ftp), still get this error 
        printf("Error: Input must begin with ftp\n");
        exit(1);
    }
    if (*fileName == 0){
         printf("Error: filename cannot be empty\n");
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
        clock_t start_time = clock(); //calculate RTT
        int num_bytes = sendto(sockfd,"ftp",strlen("ftp"),0,(struct sockaddr *)&server,sizeof(server));
        if (num_bytes<0){
            printf("Error: Send message 'ftp' to the server failed.\n");
            exit(1);
        }
        //Receive a message from the server:
        struct sockaddr_in from;  
        int buf_size = BUF_LEN;
        char buf[buf_size];
        memset(buf, '\0', buf_size);
        socklen_t from_len = sizeof(from);

        int num_bytes_from = recvfrom(sockfd,buf,buf_size,0,(struct sockaddr *)&from,&from_len);
        clock_t end_time = clock();
        
        double RTT = ((double)(end_time-start_time))/CLOCKS_PER_SEC;
        printf("Round-trip time from the client to the server is %f \n",RTT);

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
    

    //session 3
    //get total_frag
    FILE * file_tb_transfer;
    file_tb_transfer = fopen(fileName, "rb");
    if (file_tb_transfer == NULL) {
        printf("Error: Cannot Open the file\n");
        exit(1);
    }
    fseek(file_tb_transfer, 0, SEEK_END);
    size_t file_size = ftell(file_tb_transfer);
    fseek(file_tb_transfer, 0, SEEK_SET);
	unsigned int num_frag = (file_size/MAX_LEN) + 1;
	printf("The total number of fragments: %d.\n", num_frag);


    //divide the file into fragments
    int i=0;
    while (1){
        //packet init
        struct packet temp_packet;
        temp_packet.total_frag = num_frag;
        temp_packet.frag_no = i+1;
        temp_packet.filename = fileName;
        char filedata_buf[MAX_LEN];
        temp_packet.size = fread(filedata_buf,sizeof(char),MAX_LEN,file_tb_transfer);
        
        memcpy(temp_packet.filedata, filedata_buf, temp_packet.size );

        //Format buf
        char packet_buf[MAX_LEN];
        int written_size = sprintf(packet_buf, "%d:%d:%d:%s:", temp_packet.total_frag, temp_packet.frag_no, temp_packet.size , temp_packet.filename);

        memcpy(&packet_buf[written_size], filedata_buf,  temp_packet.size);

        //send to server
        int num_bytes = sendto(sockfd, packet_buf,  temp_packet.size + written_size, 0,(struct sockaddr *)&server, sizeof(server));
         if (num_bytes<0){
            printf("Error: Send packet to the server failed.\n");
            exit(1);
        }

        //Receive a message from the server:
        struct sockaddr_in from;  
        int buf_size = BUF_LEN;
        char buf[buf_size];
        memset(buf, '\0', buf_size);
        socklen_t from_len = sizeof(from);

        int num_bytes_from = recvfrom(sockfd,buf,buf_size,0,(struct sockaddr *)&from, &from_len);
         if (num_bytes_from<0){
            printf("Error: recvfrom\n");
            exit(1);
        }
        
        if (strcmp(buf,NACK) == 0){
            printf("Wait, rsent %d packet\n",i+1);
            fseek(file_tb_transfer, -temp_packet.size, SEEK_CUR);
            continue;
        }
        else if (strcmp(buf,ACK) == 0){
            printf("Success: Sent %d packet\n",i+1);
            if (feof(file_tb_transfer)){
                printf("Sent all fragments.\n");
                break;
            }
            i++;
            continue;
        }
        else{
            printf("Server Error\n",i+1);
            exit(1);
        }

    }

    // freeaddrinfo(server);
    close(sockfd);
    fclose(file_tb_transfer);
    return (0);

}
