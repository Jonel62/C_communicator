#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#define MAX 256
#define LOGIN 1
#define MESSAGE 2

struct message {
    long mesg_type;
    int sender_id;
    int receiver_id;
    char mesg_text[MAX];
};

int main() {
    srand(time(NULL));
    int id;
    int r = rand()%100000;
    int myid = msgget(r, 0666 | IPC_CREAT);

    key_t serverkey = ftok("server", 65);
    int sid = msgget(serverkey, 0666 | IPC_CREAT);

    struct message msg;
    msg.mesg_type = LOGIN;
    msg.sender_id = myid;
    msg.receiver_id = 0;
    printf("Enter your username: ");
    scanf("%s", msg.mesg_text);

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
        printf("From server: %s\n", msg.mesg_text);
        while (1) {
            printf("What you want to do (message=2, logout): ");
            scanf("%d", &msg.mesg_type);
            if (msg.mesg_type == MESSAGE) {
                printf("Which user?\n");
                scanf("%d", &msg.receiver_id);
                msg.sender_id = id;
                printf("Send message:");
                scanf("%s", &msg.mesg_text);
                msgsnd(sid, &msg,
                   sizeof(msg) - sizeof(long),
                   0);
            }
        }
    }
    else {
        while (1) {
            msgrcv(myid, &msg,
           sizeof(msg) - sizeof(long),
           MESSAGE, 0);
            printf("\nReceived message from user %d:\n %s\n", msg.sender_id, msg.mesg_text);
        }

    }
}
