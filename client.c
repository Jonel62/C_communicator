#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

#define MAX 256
#define NAME_LENGTH 16
#define OPTION_LENGTH 64

//communicates
#define HELP 102
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

int myid;
int cid;

void print_help() {
    printf("Available options:\n"
           "logout\n"
           "message\n"
           "user_list\n"
           "users_in_group\n"
           "make_group\n"
           "join_group\n"
           "group_message\n"
           "groups_list\n"
           "leave_group\n"
           "open_box\n");
}

void option_handler(const char option[OPTION_LENGTH], struct message* msg) {

    if (strcmp(option, "logout") == 0) {
        msg->mesg_type = LOGOUT;
    }
    else if (strcmp(option, "message") == 0) {
        msg->mesg_type = MESSAGE;
    }
    else if (strcmp(option, "user_list") == 0) {
        msg->mesg_type = USER_LIST;
    }
    else if (strcmp(option, "make_group") == 0) {
        msg->mesg_type = MAKE_GROUP;
    }
    else if (strcmp(option, "join_group") == 0) {
        msg->mesg_type = JOIN_GROUP;
    }
    else if (strcmp(option, "group_message") == 0) {
        msg->mesg_type = GROUP_MESSAGE;
    }
    else if (strcmp(option, "groups_list") == 0) {
        msg->mesg_type = GROUPS_LIST;
    }
    else if (strcmp(option, "users_in_group") == 0) {
        msg->mesg_type = GROUP_USERS;
    }
    else if (strcmp(option, "leave_group") == 0) {
        msg->mesg_type = LEAVE_GROUP;
    }
    else if (strcmp(option, "open_box") == 0) {
        msg->mesg_type = OPEN_BOX;
    }
    else if (strcmp(option, "help") == 0) {
        print_help();
        msg->mesg_type = HELP;
    }
}

void handle_sigint(int sig) {
    printf("\nRecived signal %d. Deleting queue...\n", sig);

    if (msgctl(myid, IPC_RMID, NULL) == -1) {
        perror("Queue error");
        exit(1);
    }

    printf("Queue %d was deleted\nQuitting...\n", myid);
    exit(0);
}

void handle_sig(int sig) {
    printf("\nRecived signal %d.", sig);

    kill(cid, SIGTERM);

    printf("Quitting...\n", myid);
    exit(0);
}

void send_message(struct message* msg, int sender_id, int sid) {
    printf("Which user?\n");
    scanf("%s", &msg->name);
    msg->sender_id = sender_id;
    printf("Send message:");
    getchar();
    fgets(msg->mesg_text, MAX, stdin);

    msgsnd(sid, msg,
       sizeof(*msg) - sizeof(long),
       0);
}

void send_server_message(struct message* msg, int id, int sid) {
    msg->sender_id = id;
    msg->receiver_id = -1;
    msgsnd(sid, msg,
    sizeof(*msg) - sizeof(long),0);
}

void heartbeat(struct message* msg, int id, int sid) {
    sleep(3);
    msg->sender_id = id;
    msg->receiver_id = -1;
    strcpy(msg->mesg_text, "still alive");
    msg->mesg_type = HEART_BEAT;
    msgsnd(sid, msg,
       sizeof(*msg) - sizeof(long),
       0);
}

void recieve_message(struct message* msg) {
    msgrcv(myid, msg,
   sizeof(*msg) - sizeof(long),
   0, 0);
    if (msg->mesg_type == MESSAGE) {
        printf("\nReceived message from user %s:\n %s\n", msg->name, msg->mesg_text);
    }
    else if (msg->mesg_type == SERVER_COMMUNICATE) {
        printf("\nReceived server communicate:\n %s\n", msg->mesg_text);
    }
    else if (msg->mesg_type == IN_BOX_MESSAGE) {
        printf("\nReceived in box message from %s:\n %s\n", msg->name, msg->mesg_text);
    }
}

void login(struct message* msg, int id, int sid) {
    msg->mesg_type = LOGIN;
    msg->sender_id = myid;
    msg->receiver_id = 0;
    printf("Enter your username: ");
    scanf("%s", msg->mesg_text);
    char name[NAME_LENGTH];
    strcpy(name, msg->mesg_text);
    msgsnd(sid, msg,
           sizeof(*msg) - sizeof(long),
           0);
    printf("Waiting for server...\n");
    msgrcv(myid, msg,
           sizeof(*msg) - sizeof(long),
           0, 0);
    if (msg->mesg_type==ERROR) {
        printf("loggin failed");
        exit(1);
    }
}

int main() {
    srand(time(NULL));
    int id;
    char option[OPTION_LENGTH];
    int r = rand()%100000;
    myid = msgget(r, 0666 | IPC_CREAT);
    key_t serverkey = ftok("server", 65);
    int sid = msgget(serverkey, 0666);
    struct message msg;

    login(&msg, id, sid);

    printf("Your id is: %d\n", msg.receiver_id);
    id=msg.receiver_id;
    int p=fork();
    if (p != 0) {
        signal(SIGINT, handle_sigint);
        signal(SIGTERM, handle_sigint);

        printf("From server: %s\n", msg.mesg_text);
        while (1) {
            printf("What do you want to do (help): ");
            scanf("%s", option);
            option_handler(&option, &msg);
            switch (msg.mesg_type) {
                case MESSAGE:
                    send_message(&msg, id, sid);
                    sleep(1);
                    break;

                case USER_LIST:
                case GROUPS_LIST:
                case OPEN_BOX:
                    send_server_message(&msg, id, sid);
                    if (msg.mesg_type != OPEN_BOX) sleep(1);
                    sleep(1);
                    break;

                case MAKE_GROUP:
                    printf("Enter group name: ");
                    scanf("%s", msg.mesg_text);
                    send_server_message(&msg, id, sid);
                    sleep(1);
                    break;

                case JOIN_GROUP:
                    printf("Enter group name: ");
                    scanf("%s", msg.name);
                    send_server_message(&msg, id, sid);
                    sleep(1);
                    break;

                case GROUP_MESSAGE:
                    printf("Enter group name: ");
                    scanf("%s", msg.name);
                    printf("Enter message: ");
                    getchar(); // Wyczyszczenie bufora po scanf
                    fgets(msg.mesg_text, MAX, stdin);
                    send_server_message(&msg, id, sid);
                    sleep(1);
                    break;

                case GROUP_USERS:
                case LEAVE_GROUP:
                    printf("Enter group name: ");
                    scanf("%s", msg.name);
                    send_server_message(&msg, id, sid);
                    sleep(1);
                    break;
                case HELP:
                    break;
                case LOGOUT:
                    send_server_message(&msg, id, sid);
                    printf("Logging out\n");
                    kill(p, SIGTERM);
                    kill(getpid(), SIGTERM);
                    sleep(1);
                default:
                    printf("Unknown message type: %ld\n", msg.mesg_type);
                    break;
            }
        }
    }
    else {
            cid = fork();
            if (cid == 0) {
                while (1) {
                    recieve_message(&msg);
                }
            }
            else {
                while (1) {
                    signal(SIGTERM, handle_sig);
                    heartbeat(&msg, id, sid);
                }
            }
        }

    }
