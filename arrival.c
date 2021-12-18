
#include "local.h"


/*
 * Generates the key and then opens the message queue created before using that key
 * */
int open_queue() {

    int key, mid;

    /*
     * Get the key to open the message queue
     * */
    if ((key = ftok(".", SEED_MSQ)) == -1) {
        perror("Public message queue key generation");
        exit(-1);
    }

    /*
     * Use the key generated before to open the message queue created before in parent
     * */
    if ((mid = msgget(key, 0)) == -1) { // this mid should be sent to parent process to be sent to passengers
        perror("Public message queue creation");
        exit(-2);
    }

    return mid;

}

Passenger receive_passenger(int mid) {

    PASSENGER_MSG_QUEUE msg;

    if (msgrcv(mid, &msg, sizeof(msg.mtext), PASSENGER_INFO_MTYPE_QUEUE, 0) == -1) {
        perror("Reading from arrival queue");
        exit(-3);
    }

    return msg.mtext[0];

}


int main(int argc, char *argv[]) {

    Passenger passenger;

    int mid = open_queue(), ppid = getppid(), pid = getpid();

    //while (1) { passenger = receive_passenger(mid); }


    return 0;
}