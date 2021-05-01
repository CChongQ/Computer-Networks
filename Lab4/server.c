#include "helper_API_server.h"

int main(int argc, char const *argv[])
{
    // Get user input
    if (argc != 2){
        printf("Invalid input format. example format: server <UDP listen port>!\n");
        exit(1);
    }
    char *port_num = malloc(sizeof(char) * strlen(argv[1]));
    strcpy(port_num, argv[1]);

    printf("Server: connecting to port %s...\n", port_num);

    // Setup server
    int sockfd; // listen on sock_fd, new connection on new_fd

    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if ((rv = getaddrinfo(NULL, port_num, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop to find available socket to bind
    for (p = servinfo; p != NULL; p = p->ai_next){
        if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1){
            perror("Server: socket!");
            continue;
        }
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,sizeof(int)) == -1){
            perror("setsockopt");
            exit(1);
        }
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            perror("Server: bind");
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo); // all done with this structure
    if (p == NULL){
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }
    // Listen on incoming port
    if (listen(sockfd, BACKLOG) == -1){
        perror("listen");
        exit(1);
    }
    printf("server: waiting client connections...\n");
    FILE *F;
    if ((F = fopen(REGISTERFILE, "r")) == NULL){
        fprintf(stderr, "Cannot open register f %s\n", REGISTERFILE);
    }
    registered_users = generate_user_list(F);
    fclose(F);
    //loop to listen from clients
    while (1){
        // Accept new incoming connections
        User *user_account = calloc(sizeof(User), 1);
        pthread_t client_thread;
        sin_size = sizeof(their_addr);
        user_account->sockfd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (user_account->sockfd == -1){
            perror("server accept error");
            break;
        }
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof(s));
        printf("server: connecting to %s\n", s);

        // Create new thread to handle the new socket
        pthread_create(&client_thread, NULL, process_requests, (void *)user_account);
    }

    // Free global memory on exit
    clean_up(registered_users);
    clean_up(currently_online);
    clean_up_session(curretly_active_sessions);
    free(port_num);
    close(sockfd);
    printf("Server closed!\n");
    return 0;
}