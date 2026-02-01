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

//communicates
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

struct message {
    long mesg_type;
    int sender_id;
    int receiver_id;
    char name[NAME_LENGTH];
    char mesg_text[MAX];
};

int myid;

void handle_sigint(int sig) {
    printf("\nRecived signal %d. Deleting queue...\n", sig);

    if (msgctl(myid, IPC_RMID, NULL) == -1) {
        perror("Queue error");
        exit(1);
    }

    printf("Queue %d was deleted\nQuitting...\n", myid);
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
}

int main() {
    srand(time(NULL));
    int id;
    int r = rand()%100000;
    myid = msgget(r, 0666 | IPC_CREAT);
    key_t serverkey = ftok("server", 65);
    int sid = msgget(serverkey, 0666);

    struct message msg;
    msg.mesg_type = LOGIN;
    msg.sender_id = myid;
    msg.receiver_id = 0;
    printf("Enter your username: ");
    scanf("%s", msg.mesg_text);
    char name[NAME_LENGTH];
    strcpy(name, msg.mesg_text);

    msgsnd(sid, &msg,
           sizeof(msg) - sizeof(long),
           0);
    printf("Waiting for server...\n");
    msgrcv(myid, &msg,
           sizeof(msg) - sizeof(long),
           0, 0);
    printf("Your id is: %d\n", msg.receiver_id);
    id=msg.receiver_id;
    if (fork() == 0) {
        signal(SIGINT, handle_sigint);
        signal(SIGTERM, handle_sigint);

        printf("From server: %s\n", msg.mesg_text);
        while (1) {
            printf("What do you want to do (message=2, user_list=3, make_group=6): ");
            scanf("%ld", &msg.mesg_type);
            if (msg.mesg_type == MESSAGE) {
                send_message(&msg, id, sid);
            }
            else if (msg.mesg_type == USER_LIST) {
                msg.sender_id = id;
                msg.receiver_id = -1;
                msgsnd(sid, &msg,
                   sizeof(msg) - sizeof(long),
                   0);
                sleep(1);
            }
            else if (msg.mesg_type == MAKE_GROUP) {
                msg.sender_id = id;
                msg.receiver_id = -1;
                printf("Enter group name: ");
                scanf("%s", msg.mesg_text);
                msgsnd(sid, &msg,
                   sizeof(msg) - sizeof(long),
                   0);
                sleep(1);
            }
            else if (msg.mesg_type == GROUPS_LIST) {
                msg.sender_id = id;
                msg.receiver_id = -1;
                msgsnd(sid, &msg,
                   sizeof(msg) - sizeof(long),
                   0);
                sleep(1);
            }
            else if (msg.mesg_type == JOIN_GROUP) {
                msg.sender_id = id;
                msg.receiver_id = -1;
                printf("Enter group name: ");
                scanf("%s", msg.name);
                msgsnd(sid, &msg,
                   sizeof(msg) - sizeof(long),
                   0);
            }
            else if (msg.mesg_type == GROUP_MESSAGE) {
                msg.sender_id = id;
                msg.receiver_id = -1;
                printf("Enter group name: ");
                scanf("%s", msg.name);
                printf("Enter message: ");
                getchar();
                fgets(msg.mesg_text, MAX, stdin);
                msgsnd(sid, &msg,
                   sizeof(msg) - sizeof(long),
                   0);
            }
            else if (msg.mesg_type == GROUP_USERS) {
                msg.sender_id = id;
                msg.receiver_id = -1;
                printf("Enter group name: ");
                scanf("%s", msg.name);
                msgsnd(sid, &msg,
               sizeof(msg) - sizeof(long),
                0);
                sleep(1);
            }
            else if (msg.mesg_type == LEAVE_GROUP) {
                msg.sender_id = id;
                msg.receiver_id = -1;
                printf("Enter group name: ");
                scanf("%s", msg.name);
                msgsnd(sid, &msg,
                sizeof(msg) - sizeof(long),
                0);
                sleep(1);
            }
        }
    }
    else {
        while (1) {
            if (fork() == 0) {
                recieve_message(&msg);
            }
            else {
                while (1) {
                    sleep(5);
                    msg.sender_id = id;
                    msg.receiver_id = -1;
                    strcpy(msg.mesg_text, "still alive");
                    msg.mesg_type = HEART_BEAT;
                    msgsnd(sid, &msg,
                       sizeof(msg) - sizeof(long),
                       0);
                }
            }
        }

    }
}
