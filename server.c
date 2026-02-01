#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>

#define MAX 256
#define LOGIN 1
#define MESSAGE 2

struct user {
    int user_id;
    int queue_id;
    char name[16];
};

struct message {
    long mesg_type;
    int sender_id;
    int receiver_id;
    char mesg_text[MAX];
};

int main() {
    struct user users[10];
    key_t key = ftok("server", 65);
    int sid = msgget(key, 0666 | IPC_CREAT);

    struct message msg;

    printf("Server started...\n");
    int i = 0;
    while (1) {

        if (msgrcv(sid, &msg,
                   sizeof(msg) - sizeof(long),
                   0, 0) == -1) {
            perror("msgrcv");
            continue;
                   }
        if (msg.mesg_type == LOGIN) {
            users[i].user_id = i;
            users[i].queue_id = msg.sender_id;
            strcpy(users[i].name, msg.mesg_text);
            printf("Login from client queue: %d as %s\n", users[i].queue_id, users[i].name);
            msg.mesg_type = MESSAGE;
            msg.sender_id = 0;
            msg.receiver_id = users[i].user_id;
            strcpy(msg.mesg_text, "logged");
            if (msgsnd(users[i].queue_id, &msg,
                       sizeof(msg) - sizeof(long),
                       0) == -1) {
                perror("msgsnd");
                       }
            i++;
        }
        else if (msg.mesg_type == MESSAGE) {
            printf("Message from client: %d to %d\n", msg.sender_id, msg.receiver_id);
            if (msgsnd(users[msg.receiver_id].queue_id, &msg,
                       sizeof(msg) - sizeof(long),
                       0) == -1) {
                perror("msgsnd");
                       }
            printf("Sended msg to queue %d\n", users[msg.receiver_id].queue_id);
        }
    }
}
