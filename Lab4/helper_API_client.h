#include <netdb.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include "packet.h"
#include "user.h"
#include "session.h"
#include <sys/time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>

//Lab4

#include <stdlib.h>
#include <sys/select.h>
#define MAX_SESSION 3
// which sessions user is in
int insessions[MAX_SESSION];
// the session to send message to
int insession;
// number of sessions currently in
int session_count;
char username[MAX_NAME] = {0};

//client information
#define LOGIN_ "/login"
#define LOGOUT_ "/logout"
#define JOINSESSION_ "/joinsession"
#define LEAVESESSION_ "/leavesession"
#define CREATESESSION_ "/createsession"
#define LIST_ "/list"
#define QUIT_ "/quit"
#define OPENSESSION_ "/opensession"
#define INVITE_ "/invite"

typedef struct __Session__ Session;

pthread_mutex_t globle_user_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t invite_mutex = PTHREAD_MUTEX_INITIALIZER;

User globle_user;
char globel_buffer[BUFFERLENGTH];
bool prev_is_invite = false;
char prev_invite_session[MAX_DATA];

// =--------===================== client functions =======================-------
void *handle_response(void *socketfd_ptr);

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void login(char *pch, char **client_id, int *socketfd_p, pthread_t *handle_response_thread_p)
{
    char *password, *server_ip, *server_port;
    // tokenize the command

    pch = strtok(NULL, " ");
    *client_id = malloc(sizeof(char) * strlen(pch));
    strcpy(*client_id, pch);

    pch = strtok(NULL, " ");
    password = pch;
    pch = strtok(NULL, " ");
    server_ip = pch;
    pch = strtok(NULL, " \n");
    server_port = pch;
    if (*client_id == NULL || password == NULL || server_ip == NULL || server_port == NULL)
    {
        fprintf(stdout, "Please retry. Usage: /login <client_id> <password> <server_ip> <server_port>\n");
        return;
    }
    else if (*socketfd_p != -1)
    {
        fprintf(stdout, "-------You can only login to 1 server simutaneously.\n");
        return;
    }
    else
    {
        // prepare to connect through TCP protocol
        int rv;

        struct addrinfo hints, *servinfo, *p;
        char s[INET6_ADDRSTRLEN];
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;

        if ((rv = getaddrinfo(server_ip, server_port, &hints, &servinfo)) != 0)
        {
            fprintf(stderr, "login, getaddrinfo: %s\n", gai_strerror(rv));
            return;
        }
        for (p = servinfo; p != NULL; p = p->ai_next)
        {
            if ((*socketfd_p = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            {
                fprintf(stderr, "client - login: socket failed\n");
                continue;
            }
            if (connect(*socketfd_p, p->ai_addr, p->ai_addrlen) == -1)
            {
                close(*socketfd_p);
                fprintf(stderr, "client - login: connect failed\n");
                continue;
            }
            break;
        }
        if (p == NULL)
        {
            fprintf(stderr, "client - login: failed to connect from addrinfo\n");
            close(*socketfd_p);
            *socketfd_p = -1;
            return;
        }
        inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
        printf("-------Connecting to %s\n", s);
        freeaddrinfo(servinfo); // all done with this structure

        int numbytes;
        Packet packet;
        packet.type = LOGIN;
        strncpy(packet.source, *client_id, MAX_NAME - 1);
        strncpy(packet.data, password, MAX_DATA - 1);
        packet.size = strlen(packet.data);
        encode(&packet, globel_buffer);
        if ((numbytes = send(*socketfd_p, globel_buffer, BUFFERLENGTH - 1, 0)) == -1)
        {
            fprintf(stderr, "client - login: send failed\n");
            close(*socketfd_p);
            *socketfd_p = -1;
            return;
        }

        if ((numbytes = recv(*socketfd_p, globel_buffer, BUFFERLENGTH - 1, 0)) == -1)
        {
            fprintf(stderr, "client - login: recv failed\n");
            close(*socketfd_p);
            *socketfd_p = -1;
            return;
        }
        globel_buffer[numbytes] = 0;
        decode(globel_buffer, &packet);
        if (packet.type == LO_ACK && pthread_create(handle_response_thread_p, NULL, handle_response, socketfd_p) == 0)
        {
            fprintf(stdout, "-------------- <%s> Login successfully------------\n",*client_id);
            strncpy(username, packet.source, MAX_NAME - 1);
        }
        else if (packet.type == LO_NAK)
        {
            fprintf(stdout, "-------Login failed. Reasons: %s\n", packet.data);
            close(*socketfd_p);
            *socketfd_p = -1;
            return;
        }
        else
        {
            fprintf(stdout, "client: Unknown packet!\n");
            close(*socketfd_p);
            *socketfd_p = -1;
            return;
        }
    }
}

void logout(int *socketfd_p, char *client_id, pthread_t *handle_response_thread_p)
{
    if (*socketfd_p == -1)
    {
        fprintf(stdout, "-------Cannot logout. You need to login first.\n");
        return;
    }

    int numbytes;
    Packet packet;
    packet.type = EXIT;
    packet.size = 0;
    strcpy(packet.source, client_id);
    encode(&packet, globel_buffer);

    if ((numbytes = send(*socketfd_p, globel_buffer, BUFFERLENGTH - 1, 0)) == -1)
    {
        fprintf(stderr, "client - logout: send failed\n");
        return;
    }
    if (pthread_cancel(*handle_response_thread_p))
    {
        fprintf(stderr, "client - logout: logout failed!\n");
    }
    else
    {
        fprintf(stdout, "--------------Logout successfully------------\n");
    }

    insession = -1;
    for (int i = 0; i < MAX_SESSION; i++)
    {
        insessions[i] = -1;
    }
    session_count = 0;
    username[0] = 0;

    close(*socketfd_p);
    *socketfd_p = -1;
}

void JoinSession(char *pch, char *client_id, int *socketfd_p)
{
    if (*socketfd_p == -1)
    {
        fprintf(stdout, "-------Error: You have not logged in.\n");
        return;
    }

    char *session_id = NULL;
    pch = strtok(NULL, "\n");
    session_id = pch;
    if (session_id == NULL)
    {
        fprintf(stdout, "-------Please retry. usage: /joinsession <session_id>\n");
    }
    else
    {
        int numbytes;
        Packet packet;
        packet.type = JOIN;
        strcpy(packet.source, client_id);
        strncpy(packet.data, session_id, MAX_DATA);
        packet.size = strlen(packet.data);
        encode(&packet, globel_buffer);
        if ((numbytes = send(*socketfd_p, globel_buffer, BUFFERLENGTH - 1, 0)) == -1)
        {
            fprintf(stderr, "client - JoinSession: send failed\n");
            return;
        }
    }
}

void createsession(char *pch, char *client_id, int socketfd)
{
    // printf("in creatning session: client id %s\n", client_id);

    if (socketfd == -1)
    {
        fprintf(stdout, "-------Error: You have not logged in.\n");
        return;
    }

    if (session_count >= MAX_SESSION)
    {
        fprintf(stdout, "-------Error: Created session failed. Reason: You have exceed the maximum ammount of allowed sessions.\n");
        return;
    }

    char *session_id = NULL;
    pch = strtok(NULL, "\n");
    session_id = pch;
    if (session_id == NULL)
    {
        fprintf(stdout, "-------Please retry. Usage: /createsession <session_id>\n");
    }
    else
    {
        int numbytes;
        Packet packet;
        packet.type = NEW_SESS;
        strcpy(packet.source, client_id);
        strncpy(packet.data, session_id, MAX_DATA);
        packet.size = strlen(packet.data);
        encode(&packet, globel_buffer);
        // printf("ready to send: %s", globel_buffer);
        if ((numbytes = send(socketfd, globel_buffer, BUFFERLENGTH - 1, 0)) == -1)
        {
            fprintf(stderr, "client - createsession: send failed\n");
            return;
        }
    }
}

void List(int socketfd, char *client_id)
{
    if (socketfd == -1)
    {
        fprintf(stdout, "-------You have not logged in.\n");
        return;
    }

    int numbytes;
    Packet packet;
    packet.type = QUERY;
    packet.size = 0;
    strcpy(packet.source, client_id);
    encode(&packet, globel_buffer);

    if ((numbytes = send(socketfd, globel_buffer, BUFFERLENGTH - 1, 0)) == -1)
    {
        fprintf(stderr, "client - List: send failed\n");
        return;
    }
}

// void leavesession(int socketfd, char *client_id){
void leavesession(int socketfd, char *pch)
{

    pch = strtok(NULL, " ");
    if (pch == NULL)
    {
        fprintf(stdout, "-------Please enter session id. Usage: /leavesession <session id>");
        return;
    }
    int session_id = atoi(pch);

    printf("-------Client request to leave session <%d>\n", session_id);

    if (socketfd == -1)
    {
        fprintf(stdout, "-------Error: You have not logged in.\n");
        return;
    }

    bool valid = false;
    int idx = 0;
    for (; idx < MAX_SESSION; idx++)
    {
        // printf("Current check session = %d\n", insessions[idx]);
        if (session_id == insessions[idx])
        {
            valid = true;
            break;
        }
    }

    if (valid)
    {
        int numbytes;
        Packet packet;
        packet.type = LEAVE_SESS;
        packet.size = 0;
        strncpy(packet.source, pch, MAX_DATA);
        encode(&packet, globel_buffer);
        if ((numbytes = send(socketfd, globel_buffer, BUFFERLENGTH - 1, 0)) == -1)
        {
            fprintf(stderr, "client - leavesession : send failed\n");
            return;
        }
        session_count--;
        fprintf(stdout, "-------You have left session <%d>.\n", session_id);
        insessions[idx] = -1;
        if (insession == session_id)
        {
            insession = -1;
        }
    }
    else
    {
        fprintf(stdout, "-------Error:You are not currently in session %d.\n", session_id);
    }
}

void send_text(int socketfd, char *client_id)
{
    // printf("Trying to send Text\n");
    if (socketfd == -1)
    {
        fprintf(stdout, "-------Error: You have not logged in.\n");
        return;
    }

    if (insession == -1)
    {
        fprintf(stdout, "-------Error: You need to join a session first.\n");
        return;
    }

    int numbytes;
    Packet packet;
    packet.type = MESSAGE;

    numbytes = snprintf(packet.source, MAX_DATA - 1, "%d", insession);

    // strcpy(packet.source, client_id);
    char insession_str[12];
    sprintf(insession_str, "%d", insession);
    char *temp = &insession_str;
    strcpy(packet.source, temp);
    strncpy(packet.data, globel_buffer, MAX_DATA);
    packet.size = strlen(packet.data);
    encode(&packet, globel_buffer);
    if ((numbytes = send(socketfd, globel_buffer, BUFFERLENGTH - 1, 0)) == -1)
    {
        fprintf(stderr, "client - send_text: send failed\n");
        return;
    }
}

void opensession(int socketfd, char *pch)
{

    if (socketfd == -1)
    {
        fprintf(stdout, "-------Error: You have not logged in.\n");
        return;
    }

    pch = strtok(NULL, " ");
    if (pch == NULL)
    {
        fprintf(stdout, "-------Please enter session id. Usage: /opensession <session id>\n");
        return;
    }

    int session_id = atoi(pch);

    printf("-------Client request to open session %d\n", session_id);

    bool found = false;
    for (int i = 0; i < MAX_SESSION; i++)
    {
        if (session_id == insessions[i])
        {
            found = true;
            insession = session_id;
            fprintf(stdout, "-------You are currently on session <%d>.\n", session_id);
            return;
        }
    }

    if (found == false)
    {
        fprintf(stdout, "-------Error: Cannot open the session. You need to join the session first\n");
        return;
    }
}

void invite(int socketfd, char *pch)
{
    if (socketfd == -1)
    {
        fprintf(stdout, "-------Error: You have not logged in.\n");
        return;
    }

    char *user_id, *session_id;
    pch = strtok(NULL, " ");
    user_id = pch;
    pch = strtok(NULL, " ");
    session_id = pch;
    if (user_id == NULL || session_id == NULL)
    {
        fprintf(stdout, "-------Please retry. Usage: /invite <user id> <session id>\n");
        return;
    }

    //check: user in the session
    bool valid = false;
    for (int i = 0; i < MAX_SESSION; i++)
    {
        if (insessions[i] == atoi(session_id))
        {
            valid = true;
            break;
        }
    }
    if (valid == false)
    {
        fprintf(stdout, "-------Error: Invitation failed. Reason: you need to join session <%s> first.\n",session_id);
        return;
    }

    int numbytes;
    Packet packet;
    packet.type = INVITE;
    packet.size = 0;
    strncpy(packet.source, user_id, MAX_NAME - 1);
    strncpy(packet.data, session_id, MAX_DATA - 1);
    packet.size = strlen(packet.data);
    encode(&packet, globel_buffer);

    if ((numbytes = send(socketfd, globel_buffer, BUFFERLENGTH - 1, 0)) == -1)
    {
        fprintf(stderr, "client - invite: send failed\n");
        return;
    }
    else{
         printf(" If the user <%s> has registered and is online, the invitation has been sent to the user. \n",packet.source);
    }
}

void *handle_response(void *socketfd_ptr)
{
    //handle receive responses from server side
    int *socketfd_p = (int *)socketfd_ptr;
    int numbytes;
    Packet packet;
    while (true)
    {
        if ((numbytes = recv(*socketfd_p, globel_buffer, BUFFERLENGTH - 1, 0)) == -1)
        {
            fprintf(stderr, "client: handle_response: recv\n");
            return NULL;
        }
        globel_buffer[numbytes] = 0;
        decode(globel_buffer, &packet);
        if (packet.type == JN_ACK)
        {
            fprintf(stdout, "-------Joined session <%s> successfully.\n", packet.data);

            insession = atoi(packet.data);
            insessions[session_count] = insession;
            session_count++;
        }
        else if (packet.type == NS_ACK)
        {
            fprintf(stdout, "-------Created and joined session <%s> successfully.\n", packet.data);

            insession = atoi(packet.data);
            insessions[session_count] = insession;
            session_count++;
        }
        else if (packet.type == JN_NAK)
        {
            fprintf(stdout, "-------Error: Join failed. Reason: %s\n", packet.data);
            insession = -1;
        }
        else if (packet.type == QU_ACK)
        {

            if (insession != -1)
            {
                fprintf(stdout, "-------You are currently in session <%d>.\n", insession);
            }
            else
            {
                fprintf(stdout, "-------You are not in any session.\n");
            }

            fprintf(stdout, "user ID  ||  joined session IDs\n%s", packet.data);
        }
        else if (packet.type == MESSAGE)
        {
            fprintf(stdout, "%s: %s\n", packet.source, packet.data);
        }
        else if (packet.type == INVITE)
        {
            pthread_mutex_lock(&invite_mutex);
            fprintf(stdout, " User <%s> is inviting you to join session <%s>, accept? (y/n)\n", packet.source, packet.data);
            prev_is_invite = true;
            strncpy(prev_invite_session, packet.data, MAX_DATA - 1);
            pthread_mutex_unlock(&invite_mutex);
        }
        else
        {
            fprintf(stdout, "client: Unexpected packet handle_response: type %d, data %s\n",
                    packet.type, packet.data);
        }
        fflush(stdout); //clear output each time
    }
    return NULL;
}