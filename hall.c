
#include "local.h"

int H_min, H_max;
int passenger_no;
int current_no = 0;
int full_hall = 0;

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


HALL_MSG_QUEUE receive_attr(int mid) {

    HALL_MSG_QUEUE msg;

    if (msgrcv(mid, &msg, sizeof(msg.mtext), HALL_ATTR_MTYPE_QUEUE, 0) == -1) {
        perror("Reading from arrival queue");
        exit(-3);
    }

    return msg;

}

void set_hall_attr (HALL_MSG_QUEUE msg) {

    H_min = msg.mtext[0];
    H_max = msg.mtext[1];

}

PASSENGER_MSG_QUEUE receive_passenger(int mid) {

    PASSENGER_MSG_QUEUE msg;

    if (msgrcv(mid, &msg, sizeof(msg.mtext), PASSENGER_HALL_MSG_QUEUE, 0) == -1) {
        perror("Reading from arrival queue");
        exit(-3);
    }

    return msg;
}


void send_passenger_to_bus (int mid, Passenger passenger) {

    PASSENGER_MSG_QUEUE msg;

    msg.mtype = BUS_PASSENGER_MTYPE_QUEUE;
    msg.mtext[0] = passenger;

    if(msgsnd(mid, &msg, sizeof(msg.mtext), 0) == -1 ) {
        perror("Reading from queue");
        exit(-3);
    }

}

int open_shmem () {

    int key,shmid;

    if ((key = ftok(".", SEED_SHM)) == -1) {
        perror("Public key generation");
        exit(-1);
    }

    if ((shmid = shmget(key,0, 0)) == -1) { // this mid should be sent to parent process to be sent to passengers
        perror("shared memory opening");
        exit(-2);
    }
    return shmid;

}

void * shmem_attach(int shmid)
{
    void * shmptr;
    if ((shmptr = shmat(shmid,0,0)) == (char *) -1) {
        perror("attaching to an address");
        exit(-5);
    }
    return shmptr;
}

/*
 * initialize the global variable current_no to
 * the shared memory segment
 * */
void attach_global_variable_to_shm (int *shmptr) {
    shmptr[CN_INDEX] = current_no;
}

int open_semaphore () {

    int key,semid;

    /*
     * Get the key to open the public semaphore
     * */
    if ((key = ftok(".", SEED_SEM)) == -1) {
        perror("Public message queue key generation");
        exit(-1);
    }
    if ((semid = semget(key,4,0)) == -1) {
        perror("semaphore error");
        exit(-6);
    }

    return semid;
}

void send_signal(char c) {
    if (c == 'f') kill(getppid(),SIGALRM);  // stop
    else if (c == 'n') kill(getppid(),SIGBUS);  // cont
}

/*
 * If the hall wasn't full and its number exceeded the H_max then consider it full
 * If the hall was full and its number fall down below H_min then consider it not full
 * */
void check_full (int current) {

    if (!full_hall && current >= H_max) {
        full_hall=1;
        send_signal('f');
    }
    else if (full_hall && current <= H_min ) {
        full_hall=0;
        send_signal('n');
    }
}

void operate_critical_section (int semid, int mid, int* shmptr ) {
    int current =0 ; 
    Passenger passenger = receive_passenger(mid).mtext[0];
    send_passenger_to_bus(mid, passenger);

    acquire.sem_num = CN_INDEX;
    if(semop(semid,&acquire,1) == -1) {
        perror("acquiring sem in hall");
        exit(-11);
    }

    // start of  critical section
    shmptr[CN_INDEX] = shmptr[CN_INDEX] + 1 ;
    current = shmptr[CN_INDEX]; 
    check_full(current);

    printf(YELLOW"Number of passengers in hall %d\npassenger %d just entered\n"RESET,shmptr[CN_INDEX],passenger.pid);
    fflush(stdout);

    // end of critical section

    release.sem_num = CN_INDEX;
    if(semop(semid,&release,1) == -1) {
        perror("releasing sem in hall");
        exit(-12);
    }

}

void signal_catcher(int id )
{
    exit(0);
    return;
}
void signal_sensitive()
 {
     if (sigset(SIGUSR1, signal_catcher) == -1) {
        perror("Cannot set SIGUSR1");
        exit(SIGINT);
    }
 }

int main (int argc, char *argv[]) {

    int mid, shmid,semid;
    int * shmptr;

    signal_sensitive();
    semid  = open_semaphore();


    mid    = open_queue();
    set_hall_attr(receive_attr(mid));

    shmid  = open_shmem();

    shmptr = (int*) shmem_attach(shmid);
    attach_global_variable_to_shm(shmptr);

    while (1){
        operate_critical_section(semid, mid, shmptr);
    }

}