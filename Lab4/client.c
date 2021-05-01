#include "helper_API_client.h"

//client main file
int main(int argc, char const *argv[])
{
    char *word_seg;
    int cut_len;
    int socketfd = -1;
    pthread_t receive_thread;
    char *client_id = NULL;


    for (int i = 0; i < MAX_SESSION; i++) {
        insessions[i] = -1;
    }
    insession = -1;
    session_count = 0;

    //client main loop to process all the requests
    while (true)
    {
        fgets(globel_buffer, BUFFERLENGTH - 1, stdin);
        // set the input of newliner as end of input
        globel_buffer[strcspn(globel_buffer, "\n")] = 0;
        word_seg = globel_buffer;
        while (*word_seg == ' ')
            word_seg += 1;
        if (*word_seg == 0){
            // ignore empty command
            continue;
        }
        if (prev_is_invite == true){
            pthread_mutex_lock(&invite_mutex); 
            if (globel_buffer[0] == 'y'){
                sprintf(globel_buffer, "/joinsession %s", prev_invite_session);
                word_seg = strtok(globel_buffer, " ");
                JoinSession(word_seg, client_id, &socketfd); //join session
                prev_is_invite = false;
            }
            
            pthread_mutex_unlock(&invite_mutex);
            continue;
        }
        word_seg = strtok(globel_buffer, " ");

        // printf("In client main, client_id = %s\n",client_id);

        if (!strcmp(word_seg, LOGIN_)){
            login(word_seg, &client_id, &socketfd, &receive_thread); //login to an account
        }
        else if (!strcmp(word_seg, LOGOUT_)){
            logout(&socketfd, client_id, &receive_thread);
        }
        else if (!strcmp(word_seg, LIST_)){
            List(socketfd, client_id); //list all users information
        }
        else if (!strcmp(word_seg, LEAVESESSION_)){
            leavesession(socketfd, word_seg); // leave the session joined
        }
        else if (!strcmp(word_seg, JOINSESSION_)){
            JoinSession(word_seg, client_id, &socketfd); //join session
        }
        else if (!strcmp(word_seg, CREATESESSION_)){
            createsession(word_seg, client_id, socketfd); //create session
        }
        else if (!strcmp(word_seg, QUIT_)){
            logout(&socketfd, client_id, &receive_thread); //terminate program
            break;
        }
        else if (!strcmp(word_seg, OPENSESSION_)){
             opensession(socketfd, word_seg);
        }
        else if (!strcmp(word_seg, INVITE_)){
             invite(socketfd, word_seg);
        }
        else{
            cut_len = strlen(word_seg);
            globel_buffer[cut_len] = ' ';
            send_text(socketfd, client_id); //send message
        }
    }
    fprintf(stdout, "You have quit successfully.\n");
    if (client_id != NULL)
        free(client_id);
    return 0;
}