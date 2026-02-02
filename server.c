#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

#define NAME_LENGTH 16
#define MAX 256
#define MAX_USERS 24
#define MAX_GROUPS 16
#define MESSAGE_IN_BOX 20

//communicates
#define IN_BOX_MESSAGE 20
#define ERROR 101
#define LOGIN 1
#define MESSAGE 2
#define USER_LIST 3
#define SERVER_COMMUNICATE 4
#define HEART_BEAT 5
#define MAKE_GROUP 6
#define JOIN_GROUP 7
#define GROUP_MESSAGE 8
#define GROUPS_LIST 9
#define GROUP_USERS 10
#define LEAVE_GROUP 11
#define OPEN_BOX 12
#define LOGOUT 13

struct message {
    long mesg_type;
    int sender_id;
    int receiver_id;
    char name[NAME_LENGTH];
    char mesg_text[MAX];
};

struct user {
    int user_id;
    int queue_id;
    char name[NAME_LENGTH];
    long last_seen;
    bool is_online;
    int messages_in_box;
    struct message buffer[MESSAGE_IN_BOX];
};

const struct user user_default = {-1,-1,"", 0, false, 0};

void init_user_list(struct user* users) {
    for (int i=0; i<MAX_USERS; i++) {
        users[i] = user_default;
    }
}

struct group {
    char name[NAME_LENGTH];
    int users_id[MAX_USERS];
};

void send_communicate(char text[MAX], struct message* msg, struct user* user) {

    msg->mesg_type = SERVER_COMMUNICATE;
    strcpy(msg->mesg_text, text);
    if (msgsnd(user->queue_id, msg,
               sizeof(*msg) - sizeof(long),
               0) == -1) {
        perror("msgsnd");
               }
    printf("Sended communicate to %s\n", user->name);
}

void init_group_list(struct group* groups) {
    for (int i=0; i<MAX_USERS; i++) {
        strcpy(groups[i].name, "");
        for (int j=0; j<MAX_USERS; j++) {
            groups[i].users_id[j] = -1;
        }
    }
}

int sid;

void handle_sigint(int sig) {
    printf("\nRecived signal %d. Deleting queue...\n", sig);

    if (msgctl(sid, IPC_RMID, NULL) == -1) {
        perror("Queue error");
        exit(1);
    }

    printf("Queue %d was deleted\nQuitting...\n", sid);
    exit(0);
}

void login_user(struct message* msg, struct user* user, int* i) {
    user->user_id = *i;
    user->queue_id = msg->sender_id;
    user->last_seen = time(NULL);
    user->is_online = true;
    strcpy(user->name, msg->mesg_text);
    printf("Login from client queue: %s with id %d\n", user->name, user->user_id);
    msg->receiver_id= user->user_id;
    send_communicate("logged", msg, user);
}

int find_user(struct user* users, struct message* msg) {
    for (int j=0; j<MAX_USERS; j++) {
        if (strcmp(users[j].name, msg->mesg_text)==0) {
            return j;
        }
    }
    printf("User don't exist\n");
    return -1;
}

int find_free_group_slot(struct group* groups) {
    for (int j=0; j<MAX_GROUPS; j++) {
        if (strcmp(groups[j].name, "") == 0) {
            return j;
        }
    }
    printf("There are none slots avialable\n");
}


void send_message(struct message* msg, struct user * users) {
    printf("Message from client: %d to %d\n", msg->sender_id, msg->receiver_id);
    if (users[msg->receiver_id].is_online) {
        if (msg->mesg_type!=IN_BOX_MESSAGE)
            msg->mesg_type=MESSAGE;
        if (msgsnd(users[msg->receiver_id].queue_id, msg,
                   sizeof(*msg) - sizeof(long),
                   0) == -1) {
            perror("msgsnd");
                   }
        printf("Sended message to queue %d\n", users[msg->receiver_id].queue_id);
    }
    else {
        msg->mesg_type=IN_BOX_MESSAGE;
        users[msg->receiver_id].buffer[users[msg->receiver_id].messages_in_box]=*msg;
        users[msg->receiver_id].messages_in_box=users[msg->receiver_id].messages_in_box+1;
        printf("message sended to box\n");
    }
}

void find_receiver(struct message* msg, struct user* users) {
    for (int j = 0; j < MAX_USERS; j++) {
        if (strcmp(users[j].name, msg->name) == 0) {
            msg->receiver_id=users[j].user_id;
            break;
        }
    }
}

int find_group(struct message* msg, struct group* groups) {
    for (int j = 0; j < MAX_GROUPS; j++) {
        if (strcmp(groups[j].name, msg->name) == 0) {
            return j;
        }
    }
    printf("This group doesnt exist\n");
}

void send_group_users(struct group* group, struct user* users, struct message* msg) {
    char text[MAX]="Users in group: ";
    for (int k=0; k<MAX_USERS; k++) {
        if (group->users_id[k] >= 0) {
            strcat(text, users[group->users_id[k]].name);
            strcat(text, ", ");
        }
    }
    send_communicate(text, msg, &users[msg->sender_id]);
    printf("Sended users in group list to %s\n", users[msg->receiver_id].name);
}

void find_sender_name(struct message* msg, struct user* users) {
    for (int j = 0; j < MAX_USERS; j++) {
        if (users[j].user_id == msg->sender_id) {
            strcpy(msg->name, users[j].name);
            break;
        }
    }
}

void send_groups_list(struct message* msg, struct group* groups, struct user* users) {
    char text[MAX] = "Currently exisiting groups: ";
    for (int k=0; k<MAX_USERS; k++) {
        if (strcmp(groups[k].name, "") != 0) {
            strcat(text, groups[k].name);
            strcat(text, ", ");
        }
    }
    send_communicate(text, msg, &users[msg->sender_id]);
    printf("Sended groups list to %s\n", users[msg->sender_id].name);
}

void send_user_list(struct message* msg, struct user* users) {
    char text[MAX]="Currently logged users: ";
    for (int k=0; k<MAX_USERS; k++) {
        if (users[k].user_id >= 0 && users[k].is_online) {
            strcat(text, users[k].name);
            strcat(text, ", ");
        }
    }
    send_communicate(text, msg, &users[msg->sender_id]);
    printf("Sended user list to %s\n", users[msg->sender_id].name);
}

void logout(struct user* users,int id) {
    users[id].is_online=false;
    users[id].messages_in_box=0;
    users[id].last_seen=time(NULL);
    users[id].queue_id=-1;
    printf("User %s logged out\n", users[id].name);
}

void check_users(struct user* users) {
    for (int j = 0; j < MAX_USERS; j++) {
        if (users[j].user_id < 0 || users[j].is_online == false) {
            continue;
        }
        long last_seen = time(NULL) - users[j].last_seen;
        if (last_seen >= 10) {
            printf("User %d not responding\n", users[j].user_id, users[j].user_id);
            logout(users, j);
        }
    }
}

void make_group(int* i, struct group* group, struct message* msg) {
    strcpy(group->name, msg->mesg_text);
    group->users_id[0]=msg->sender_id;
    printf("New group %s\n", group->name);
}

int main() {

    struct group groups[MAX_GROUPS];
    struct user users[MAX_USERS];
    init_user_list(users);
    init_group_list(groups);
    FILE *file, *file1;
    char buffer[MAX];
    int n = 0;
    ///////loading_files///////
    file = fopen("users.txt", "r");

    if (file == NULL) {
        perror("File not found");
        return 1;
    }

    while (fgets(buffer, MAX, file) != NULL && n < MAX_USERS) {
        buffer[strcspn(buffer, "\r\n")] = 0;
        strcpy(users[n].name, buffer);
        users[n].user_id = n;
        n++;
    }
    fclose(file);
    ////////////////////////
    n=0;
    file = fopen("groups.txt", "r");

    if (file == NULL) {
        perror("File not found");
        return 1;
    }

    while (fgets(buffer, MAX, file) != NULL && n < MAX_GROUPS) {
        buffer[strcspn(buffer, "\r\n")] = 0;
        strcpy(groups[n].name, buffer);
        n++;
    }
    fclose(file);
    ////////////////////////
    for (int j = 0; j < MAX_GROUPS; j++) {
        printf("%s\n",groups[j].name);
    }
    key_t key = ftok("server", 65);
    sid = msgget(key, 0666 | IPC_CREAT);
    signal(SIGINT, handle_sigint);
    signal(SIGTERM, handle_sigint);
    struct message msg;

    printf("Server started...\n");
    int i = 0;
    while (1) {
        check_users(users);
        if (msgrcv(sid, &msg,
                   sizeof(msg) - sizeof(long),
                   0, 0) == -1) {
            perror("msgrcv");
            continue;
                   }
        switch (msg.mesg_type) {
            case LOGIN:
                i = find_user(users, &msg);
                if (i != -1) {
                    login_user(&msg, &users[i], &i);
                } else {
                    printf("Login failed. User not found.\n");
                    msg.mesg_type = ERROR;
                    if (msgsnd(msg.sender_id, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
                        perror("msgsnd");
                    }
                }
                break;

            case MESSAGE: {
                char buf[MAX];
                strcpy(buf, msg.mesg_text);
                send_communicate("Your message has been accepted", &msg, &users[msg.sender_id]);
                strcpy(msg.mesg_text, buf);
                find_receiver(&msg, users);
                find_sender_name(&msg, users);
                send_message(&msg, users);
                send_communicate("Your message has been delivered", &msg, &users[msg.sender_id]);
                break;
            }

            case USER_LIST:
                send_user_list(&msg, users);
                break;

            case HEART_BEAT:
                users[msg.sender_id].last_seen = time(NULL);
                break;

            case MAKE_GROUP:
                i = find_free_group_slot(groups);
                make_group(&i, &groups[i], &msg);
                break;

            case GROUPS_LIST:
                send_groups_list(&msg, groups, &users);
                break;

            case JOIN_GROUP:
                i = find_group(&msg, groups);
                groups[i].users_id[msg.sender_id] = users[msg.sender_id].user_id;
                printf("Added user %d to group %s\n", msg.sender_id, groups[i].name);
                break;

            case GROUP_MESSAGE: {
                char buf[MAX];
                printf("Message to group %s from %d\n", msg.name, msg.sender_id);
                strcpy(buf, msg.mesg_text);
                send_communicate("Your message has been accepted", &msg, &users[msg.sender_id]);
                strcpy(msg.mesg_text, buf);
                i = find_group(&msg, groups);
                find_sender_name(&msg, users);
                for (int k = 0; k < MAX_USERS; k++) {
                    if (groups[i].users_id[k] < 0 || groups[i].users_id[k] == msg.sender_id) {
                        continue;
                    }
                    msg.receiver_id = groups[i].users_id[k];
                    send_message(&msg, &users[groups[i].users_id[k]]);
                }
                send_communicate("Your message has been delivered to group", &msg, &users[msg.sender_id]);
                break;
            }

            case GROUP_USERS:
                i = find_group(&msg, groups);
                send_group_users(&groups[i], users, &msg);
                break;

            case LEAVE_GROUP:
                i = find_group(&msg, groups);
                for (int k = 0; k < MAX_USERS; k++) {
                    if (groups[i].users_id[k] == msg.sender_id) {
                        groups[i].users_id[k] = -1;
                    }
                }
                send_communicate("You've been removed from the group.\n", &msg, &users[msg.sender_id]);
                break;

            case OPEN_BOX:
                if (users[msg.sender_id].messages_in_box == 0) {
                    printf("User doesn't have messages in box\n");
                    send_communicate("You dont have any messages in box\n", &msg, &users[msg.sender_id]);
                } else {
                    for (int k = 0; k < users[msg.sender_id].messages_in_box; k++) {
                        send_message(&users[msg.sender_id].buffer[k], users);
                        users[msg.sender_id].buffer[k] = (struct message){IN_BOX_MESSAGE, -1, -1, "", ""};
                    }
                }
                break;

            case LOGOUT:
                logout(users, msg.sender_id);
                break;

            default:
                printf("Warning: Received unknown message type: %ld\n", msg.mesg_type);
                break;
        }
    }

    }