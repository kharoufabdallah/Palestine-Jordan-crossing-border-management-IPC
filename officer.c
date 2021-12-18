
#include "local.h"

char nationality;
int processing_flag = 1;

/*
 * Opens a public message queue
 * */
int open_queue() {

    int key,mid;

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

/*
 * Receives processes' attributes from queue
 * */
void receive_officer_attr(int mid) {

    OFF_ATTR_MSG_QUEUE msg;

    if (msgrcv(mid, &msg, sizeof(msg.mtext), getpid(), 0) == -1) {
        perror("Reading from arrival queue");
        exit(-3);
    }

    nationality = msg.mtext[0];

}

Passenger receive_passenger (int mid) {

    int mtype;
    PASSENGER_MSG_QUEUE msg;

    if (nationality == 'P') mtype = PAL_PASSENGER_MTYPE_QUEUE;
    else if (nationality == 'J') mtype = JOR_PASSENGER_MTYPE_QUEUE;
    else if (nationality == 'F') mtype = FOG_PASSENGER_MTYPE_QUEUE;
    else mtype = 9999;

    if (msgrcv(mid, &msg, sizeof(msg.mtext), mtype, 0) == -1) {
        perror("Reading from arrival queue");
        exit(-3);
    }
    printf(CYAN"Passenger %d received in officer %d with nationality %c\n"RESET,msg.mtext->pid,getpid(),msg.mtext->nat);
    fflush(stdout);
    fflush(stdout); 

    return msg.mtext[0]; // passenger in passenger message
}

void process_rate () {

    srand(time(NULL)-getpid()*100);
    sleep(rand()%MAX_OFFICER_PROCESS_TIME+MIN_OFFICER_PROCESS_TIME);


}

bool process_passenger (Passenger passenger) {

    process_rate();
    return passenger.valid;

}

void tell_the_passenger (int mid, Passenger passenger, bool valid) {

    VALID_MSG_QUEUE msg;

    msg.mtype = passenger.pid;
    msg.mtext[0] = valid;

    if(msgsnd(mid, &msg, sizeof(msg.mtext), 0) == -1 ) {
        perror("Reading from queue");
        exit(-3);
    }
}

void signal_catcher(int id) {
    exit(0);
}

void signal_catcher2 (int id) {
    processing_flag = 0;
    printf(RED"officer %d aren't processing passengers right now due to full hall case \n"RESET,getpid()); fflush(stdout);
}

void signal_catcher3 (int id) {
    processing_flag = 1;
    printf(CYAN"officer %d is processing passengers now due to H_min parameter\n"RESET,getpid()); fflush(stdout);
}

void signal_sensitive() {
    if (sigset(SIGUSR1, signal_catcher) == -1) {
        perror("Cannot set SIGUSR1");
        exit(SIGUSR1);
    }
    if (sigset(SIGALRM, signal_catcher2) == -1) {
        perror("Cannot set SIGUSR2");
        exit(SIGUSR2);
    }
    if (sigset(SIGBUS, signal_catcher3) == -1) {
        perror("Cannot set SIGALRM");
        exit(SIGALRM);
    }
}

int main (int argc, char *argv[]) {

    int mid;
    Passenger passenger;
    bool valid;

    signal_sensitive();

    mid = open_queue();

    receive_officer_attr(mid);


    while (1) {
        if (processing_flag) {
            passenger = receive_passenger(mid);
            valid = process_passenger(passenger);
            tell_the_passenger(mid, passenger, valid);
        }
    }


}