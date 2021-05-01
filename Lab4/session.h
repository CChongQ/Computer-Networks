#ifndef SESSION_HEAD
#define SESSION_HEAD

#include <stdbool.h>
#include <assert.h>
#include <stdio.h>


//#include "user.h"
typedef struct __User__ User;
//session structure
typedef struct __Session__ {
    char* session_ID;
    struct User *user_acc;
    struct Session *next;

} Session;

/* Check if a session is valid.
 * Return pointer to this session for in_Session()
 */
Session *search_session_by_ID(Session *currently_active_sessions, char* session_ID) {
    Session *current = currently_active_sessions;
    while(current != NULL) {
        if(strcmp(current -> session_ID , session_ID) == 0) {
            return current;
        }
        current = current -> next;
    }
    return NULL;
}


/* Initialize new session. Add new session to the head of the list
 * Session ends as last user leaves
 */
Session *insert_session(Session *currently_active_sessions, char* session_ID) {
    // printf("insert session %s\n",session_ID);
    Session *newSession = calloc(1 , sizeof(Session));
    newSession -> session_ID = malloc(sizeof(char) * strlen(session_ID));
    strcpy(newSession-> session_ID, session_ID );
    newSession->next = NULL;
   
    // Insert new session to head of the list
    if (currently_active_sessions == NULL){
        currently_active_sessions = newSession;
    }
    else{
        newSession -> next  = currently_active_sessions;
    }
    
    // printf("Insert new session id: %s\n", newSession -> session_ID);
    return newSession;
}


/* Insert new user into global session_list.
 * Have to insert user_account into session list of each corresponding user
 * in its thread.
 */

Session * enter_session(Session *currently_active_sessions, char * session_ID, const User *user_account) {
    // Check session exists outside & Find current session
    Session *cur = search_session_by_ID(currently_active_sessions, session_ID);
    assert(cur != NULL);

    // Malloc new user
    User* new_user_account = malloc(sizeof(User));
    memcpy((void *)new_user_account, (void *)user_account, sizeof(User));

    // Insert into session list
    cur -> user_acc = add_new_user(cur -> user_acc, new_user_account);

    // printf("User joined session %s\n", session_ID);
    return currently_active_sessions;
}

/* Remove a session from list.
 * Called by new_client() and exit_from_session()
 */
Session* remove_session(Session* currently_active_sessions, char* session_ID) {
    // Search for this session from currently_active_sessions
    assert(currently_active_sessions != NULL);

    // First in list
    if(strcmp(currently_active_sessions -> session_ID , session_ID) == 0) {
        Session *cur = currently_active_sessions -> next;
        free(currently_active_sessions -> session_ID);
        free(currently_active_sessions);
        return cur;
    }

    else {
        Session *cur = currently_active_sessions;
        Session *prev;
        while(cur != NULL) {
            if(strcmp(cur -> session_ID , session_ID) == 0) {
                prev -> next = cur -> next;
                free(cur -> session_ID);
                free(cur);
                break;
            } else {
                prev = cur;
                cur = cur -> next;
            }
        }
        return currently_active_sessions;
    }
}

//remove user from the session
Session * exit_from_session(Session *currently_active_sessions, char* session_ID, const User *user_account) {
    // Check session exists outside & Find current session
    // printf("in left session, session_ID : %s\n", session_ID);
    Session *cur = search_session_by_ID(currently_active_sessions, session_ID);
    // printf("in left session, cur : %d\n", cur == NULL);
    assert(cur != NULL);

    // Remove user from this session
    assert(search_user_by_name(cur -> user_acc, user_account));
    cur -> user_acc = delete_user_from_list(cur -> user_acc, user_account);
    // If last user in session, the remove session
    if(cur -> user_acc == NULL) {
        currently_active_sessions = remove_session(currently_active_sessions, session_ID);
    }
    return currently_active_sessions;
}


void clean_up_session(Session *currently_active_sessions) {
    Session *current = currently_active_sessions;
    Session *next = current;
    while(current != NULL) {
        clean_up(current -> user_acc);
        next = current -> next;
        free(current -> session_ID);
        free(current);
        current = next;
    }
}

#endif