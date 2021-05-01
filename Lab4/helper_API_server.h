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

//server information
#define BACKLOG 10                         // how many pending connections queue will hold
#define REGISTERFILE "registered_user.txt" // File of username and passwords

User *registered_users = NULL;     // List of all users and passwords
User *currently_online = NULL;     // List of users logged in
Session *curretly_active_sessions; // List of all sessions created

// Enforce synchronization
pthread_mutex_t curretly_active_sessions_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t currently_online_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct __Session__ Session;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// =--------===================== server functions =======================-------



// Subroutine to handle new connections
void *process_requests(void *arg){
    char source[MAX_NAME]; // store name

    User *user_account = (User *)arg;
    Session *joined_sessions = NULL; // List of sessions joined
    char buffer[BUFFERLENGTH];       // Buffer for recv()

    int bte_sent, bte_rec;

    Packet request;  // Convert received data into packet
    Packet response; // Packet to send

    printf("New thread created\n");

    // FSM states
    bool signed_in = 0;
    bool exit_status = 0;

    // The main recv() loop
    while (1){
        memset(buffer, 0, sizeof(char) * BUFFERLENGTH);
        memset(&request, 0, sizeof(Packet));
        memset(&response, 0, sizeof(Packet));

        if ((bte_rec = recv(user_account->sockfd, buffer, BUFFERLENGTH - 1, 0)) == -1){
            perror("process_requests error recv\n");
            exit(1);
        }
        if (bte_rec == 0)
            exit_status = 1;
        buffer[bte_rec] = '\0';

        bool Ready_to_send = 0; // Whether to send response after this loop
        // printf("server: receive : \"%s\"\n", buffer);

        decode(buffer, &request);
        memset(&response, 0, sizeof(Packet));

        // Can exit anytime
        if (request.type == EXIT){

            exit_status = 1;
        }
        // check if it is already signed in
        if (signed_in == 0){
            if (request.type == LOGIN){
                // Parse and check username and password
                int parser = 0;
                strcpy(user_account->user_name, (char *)(request.source));
                strcpy(user_account->password, (char *)(request.data));
                if (search_user_by_name(registered_users, user_account) && !search_user_by_name(currently_online, user_account)){
                    response.type = LO_ACK;
                    memcpy(response.source, request.source, MAX_DATA);
                    Ready_to_send = 1;
                    signed_in = 1;

                    // Add user to currently_online list
                    User *tmp = malloc(sizeof(User));
                    memcpy(tmp, user_account, sizeof(User));
                    pthread_mutex_lock(&currently_online_mutex);
                    currently_online = add_new_user(currently_online, tmp);
                    pthread_mutex_unlock(&currently_online_mutex);

                    // Save username
                    strcpy(source, user_account->user_name);

                    printf("User <%s> has logged in successfully\n", user_account->user_name);
                }
                else{
                    printf("Invalid User.\n");

                    response.type = LO_NAK;
                    Ready_to_send = 1;
                    // Respond with reason for failure
                    //can be changed
                    if (search_user_by_name(currently_online, user_account)){
                        strcpy((char *)(response.data), "Logged in successfully.");
                    }
                    else if (search_user_by_name(registered_users, user_account)){
                        strcpy((char *)(response.data), "Incorrect password.");
                    }
                    else{
                        strcpy((char *)(response.data), "User can not be found in registered list.");
                    }
                    //determin whether to quit or not
                    exit_status = 1;
                }
            }
            else{
                response.type = LO_NAK;
                Ready_to_send = 1;
                strcpy((char *)(response.data), "Please log in first.");
            }
        }

        //User already sigend in, want to join session
        else if (request.type == JOIN){
            char *session_ID = (char *)(request.data);
            printf("User <%s> request to join session <%s>.\n", user_account->user_name, session_ID);

            // Fails if session not exists
            if (search_session_by_ID(curretly_active_sessions, session_ID) == NULL){
                response.type = JN_NAK;
                Ready_to_send = 1;
                int parser = sprintf((char *)(response.data), "%s", session_ID);
                strcpy((char *)(response.data + parser), " Session does not exist");
                printf("User <%s> failed to join session <%s>.\n", user_account->user_name, session_ID);
            }
            // Fails if already joined session
            else if (user_account->joined_sessions != NULL){ //  Need to change this!!!!! to: check if user already joined the session instead of check whether user has joined a session
                response.type = JN_NAK;
                Ready_to_send = 1;
                int parser = sprintf((char *)(response.data), "%s", session_ID);
                strcpy((char *)(response.data + parser), "Already joined the session.");
                printf("User <%s> failed to join session <%s>.\n", user_account->user_name, session_ID);
            }
            else{// Pass error check, can join session
                // Update response JN_ACK
                response.type = JN_ACK;
                Ready_to_send = 1;
                sprintf((char *)(response.data), "%s", session_ID);

                // Update global curretly_active_sessions
                pthread_mutex_lock(&curretly_active_sessions_mutex);
                curretly_active_sessions = enter_session(curretly_active_sessions, session_ID, user_account);
                pthread_mutex_unlock(&curretly_active_sessions_mutex);

                // Update private joined_sessions
                joined_sessions = insert_session(joined_sessions, session_ID);

                printf("User <%s> joined session <%s> successfully.\n", user_account->user_name, session_ID);

                // Update user status in userConnected
                pthread_mutex_lock(&currently_online_mutex);
                for (User *user_acc = currently_online; user_acc != NULL; user_acc = user_acc->next){
                    if (strcmp(user_acc->user_name, source) == 0){//find the user
                        user_acc->joined_sessions = insert_session(user_acc->joined_sessions, session_ID);
                    }
                }
                pthread_mutex_unlock(&currently_online_mutex);
            }
        }

        // User already sigend in, want to leave session
        else if (request.type == LEAVE_SESS){
            
            char* request_sessionId = (char *)(request.source);
            printf("User <%s> request to leave session <%s>. \n", user_account->user_name,request_sessionId);

            // Iterate until left the session
            while (joined_sessions != NULL){
                char *curSessId = joined_sessions->session_ID;
                //find the session to leave
                if (strcmp(curSessId,request_sessionId) !=0 ){
                    joined_sessions = joined_sessions->next;
                    continue;
                }


                // Free private joined_sessions
                pthread_mutex_lock(&curretly_active_sessions_mutex);
                curretly_active_sessions = exit_from_session(curretly_active_sessions, curSessId, user_account);
                pthread_mutex_unlock(&curretly_active_sessions_mutex);
                printf("User <%s> left session <%s>.\n", user_account->user_name, curSessId);

                Session *cur = joined_sessions;
                joined_sessions = joined_sessions->next;
                free(cur->session_ID);
                free(cur);
               
                break;
                
            }

            encode(&response, buffer);
            // Update currently online user list
            pthread_mutex_lock(&currently_online_mutex);
            for (User *user_acc = currently_online; user_acc != NULL; user_acc = user_acc->next){
                if (strcmp(user_acc->user_name, source) == 0){ //find the left user
                    // clean_up_session(user_acc->joined_sessions);
                    // user_acc->joined_sessions = NULL;

                    user_acc ->joined_sessions = remove_session(user_acc ->joined_sessions ,request_sessionId);
                    break;
                }
            }
            pthread_mutex_unlock(&currently_online_mutex);
        }

        // User already sigend in, want to create new session
        else if (request.type == NEW_SESS){
            printf("User <%s> request to create new session.\n", user_account->user_name);
            char *session_ID = (char *)(request.data);

            pthread_mutex_lock(&curretly_active_sessions_mutex);
            curretly_active_sessions = insert_session(curretly_active_sessions, session_ID);
            pthread_mutex_unlock(&curretly_active_sessions_mutex);

            // User join a newly created session
            joined_sessions = insert_session(joined_sessions, session_ID);
            pthread_mutex_lock(&curretly_active_sessions_mutex);
            curretly_active_sessions = enter_session(curretly_active_sessions, session_ID, user_account);
            pthread_mutex_unlock(&curretly_active_sessions_mutex);

            // Update user status in currently_online;
            pthread_mutex_lock(&currently_online_mutex);
            for (User *user_acc = currently_online; user_acc != NULL; user_acc = user_acc->next){
                // printf("in user loop\n");
                if (strcmp(user_acc->user_name, source) == 0){
                    user_acc->joined_sessions = insert_session(user_acc->joined_sessions, session_ID);
                }
            }
            pthread_mutex_unlock(&currently_online_mutex);

            // Update response NS_ACK
            response.type = NS_ACK;
            Ready_to_send = 1;
            sprintf((char *)(response.data), "%s", session_ID);

            printf("User <%s> created session <%s> successfully\n", user_account->user_name, session_ID);
        }

        // User already sigend in, want to send text messages
        else if (request.type == MESSAGE){
            printf("User <%s> is sending message: \"%s\"\n", user_account->user_name, request.data);

            // Prepare message to be sent
            memset(&response, 0, sizeof(Packet));
            response.type = MESSAGE;

            char temp_source[MAX_DATA];
            snprintf(temp_source, MAX_DATA,"%s (from session %s)",user_account->user_name,request.source);

            strcpy((char *)(response.source), &temp_source);
            strcpy((char *)(response.data), (char *)(request.data));
            response.size = strlen((char *)(response.data));

            //find user joined session
            char* user_current_session = (char *)(request.source);

            // Use recv() buffer
            memset(buffer, 0, sizeof(char) * BUFFERLENGTH);
            encode(&response, buffer);
            fprintf(stderr, "Server: sending message < %s >from client <%s> to session <%s>.\n",  buffer ,user_account->user_name,user_current_session);

            // Send though local session list
            for (Session *cur = joined_sessions; cur != NULL; cur = cur->next){
                /* Send this message to all users in this session.
                 * User may receive duplicate messages.
                 */
                if ( strcmp(cur->session_ID,user_current_session) !=0){
                    continue;
                }

                // Find corresponding session in global curretly_active_sessions
                Session *sessReady_to_send;
                if ((sessReady_to_send = search_session_by_ID(curretly_active_sessions, cur->session_ID)) == NULL)//cannot find the session
                    continue;
                //loop through all users in the session
                for (User *user_acc = sessReady_to_send->user_acc; user_acc != NULL; user_acc = user_acc->next){
                    //Corner case: prevent send to self
                    if(strcmp(user_acc ->user_name, source) == 0) continue;

                    if ((bte_sent = send(user_acc->sockfd, buffer, BUFFERLENGTH - 1, 0)) == -1){
                        perror("error sending messages!\n");
                        exit(1);
                    }
                }
            }
            Ready_to_send = 0;
        }

        // Query session
        else if (request.type == QUERY){

            printf("User <%s> request to query list \n", user_account->user_name);

            int parser = 0;
            response.type = QU_ACK;
            Ready_to_send = 1;

            for (User *user_acc = currently_online; user_acc != NULL; user_acc = user_acc->next){
                parser += sprintf((char *)(response.data) + parser, "%s  ----- ", user_acc->user_name);

                Session* tempsession = user_acc->joined_sessions ;
                while (tempsession != NULL){
                    parser += sprintf((char *)(response.data) + parser, "%s; ", tempsession->session_ID);
                    tempsession = tempsession->next;
                }
                response.data[parser++] = '\n';
            }

            printf("-------query result: %s\n", response.data);
        }

        else if (request.type == INVITE){
            printf("User <%s> request to invite user <%s> to session <%s>.\n",source,request.source,request.data);
            response = request;
            memcpy(response.source, source, MAX_NAME);
            memset(buffer, 0, sizeof(char) * BUFFERLENGTH);
            encode(&response, buffer);

            for (User *user_acc = currently_online; user_acc != NULL; user_acc = user_acc->next){
                if(strcmp(request.source, user_acc->user_name) == 0) {
                    if ((bte_sent = send(user_acc->sockfd, buffer, BUFFERLENGTH - 1, 0)) == -1){
                        perror("error sending messages!\n");
                        exit(1);
                    }
                    printf(" If the user <%s> has registered, the invitation has been sent to the user. \n",user_acc->user_name);
                }
            }
        }



        if (Ready_to_send){
            // Add source and size for response and send packet
            memcpy(response.source, user_account->user_name, USERNAMELENGTH);
            response.size = strlen((char *)(response.data));

            memset(buffer, 0, BUFFERLENGTH);
            encode(&response, buffer);
            if ((bte_rec = send(user_account->sockfd, buffer, BUFFERLENGTH - 1, 0)) == -1)
            {
                perror("server error send\n");
            }
        }
        if (exit_status)
            break;
    }

    //  exits from process server
    close(user_account->sockfd);

    // User(s) alrady signed in, cleanups
    if (signed_in == 1){
        // clean up all the session before leaving
        for (Session *cur = joined_sessions; cur != NULL; cur = cur->next){
            pthread_mutex_lock(&curretly_active_sessions_mutex);
            curretly_active_sessions = exit_from_session(curretly_active_sessions, cur->session_ID, user_account);
            pthread_mutex_unlock(&curretly_active_sessions_mutex);
        }

        // Remove from global currently_online
        for (User *user_acc = currently_online; user_acc != NULL; user_acc = user_acc->next){
            if (strcmp(source, user_acc->user_name) == 0)
            {
                clean_up_session(user_acc->joined_sessions);
                break;
            }
        }
        currently_online = delete_user_from_list(currently_online, user_account);

        // clean up all sessions
        clean_up_session(joined_sessions);
        free(user_account);
    }

    if (signed_in){
        printf("User <%s> request to quit\n\n", source);
    }
    else{
        printf("User quiting\n\n");
    }
    signed_in = 0;

    return NULL;
}