#ifndef USER_HEAD
#define USER_HEAD

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <assert.h>


#define PASSWORDLEN 32
#define USERNAMELENGTH 32

typedef struct __Session__ Session;
//user structure
typedef struct __User__ {
    int sockfd;
    Session *joined_sessions;
    struct User *next;
    char user_name[USERNAMELENGTH];
    char password[PASSWORDLEN];

} User;

// Add new ones
User *add_new_user(User* user_queue, User *newuser_account) {
    newuser_account -> next = user_queue;
    return newuser_account;
}

// delete users
User *delete_user_from_list(User *user_queue, User const *user_account) {
    if(user_queue == NULL) return NULL;
    else if(strcmp(user_queue -> user_name, user_account -> user_name) == 0) {
        User *cur = user_queue -> next;
        free(user_queue);
        return cur;
    }
    else {
        User *cur = user_queue;
        User *prev;
        while(cur != NULL) {
            if(strcmp(cur -> user_name, user_account -> user_name) == 0) {
                prev -> next = cur -> next;
                free(cur);
                break;
            } else {
                prev = cur;
                cur = cur -> next;
            } 
        }
        return user_queue;
    }
}




// search a user based on name
bool search_user_by_name(const User *user_queue, const User *user_account) {
    const User *current = user_queue;
    while(current != NULL) {
        if(strcmp(current -> user_name, user_account -> user_name) == 0) { // find the username
            return 1;    
        }
        current = current -> next;
    }
    return 0;
}
// parse input register file
User * generate_user_list(FILE * F) {
    int cur;
    User *head_node = NULL;

    while(1) {
        User *user_account = calloc(1, sizeof(User));
        cur = fscanf(F, "%s %s\n", user_account -> user_name, user_account -> password);
        if(cur == EOF) {
            free(user_account);
            break;
        }
        // user_account -> next = head_node;
        // head_node = user_account;
        head_node = add_new_user(head_node, user_account);
    }
    return head_node;
}
// user destructor
void clean_up(User *head_node) {
    User *current = head_node;
    User *next = current;
    while(current != NULL) {
        next = current -> next;
        free(current);
        current = next;
    }
}
#endif