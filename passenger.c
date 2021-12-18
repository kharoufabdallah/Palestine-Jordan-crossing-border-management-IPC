#include "local.h"

Passenger *passenger;

void alarm_catcher(int id);
void signal_catcher(int id);

void signal_sensitive() {
    if (sigset(SIGALRM, alarm_catcher) == -1) {
        perror("Cannot set SIGALARM");
        exit(SIGALRM);
    }

    if (sigset(SIGUSR1, signal_catcher) == -1) {
        perror("Cannot set SIGALARM");
        exit(SIGUSR1);
    }
}

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
    if ((shmptr = shmat(shmid,0,0)) == (void *) -1) {
        perror("attaching to an address");
        exit(-5);
    }
    return shmptr;
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

/*
 * Sets the attributes of the global struct mtext
 * */
void set_passenger_attr(const int attr[4]) {
    int pal_perc = attr[0];
    int jor_perc = attr[1];
    int fog_perc = attr[2];
    int valid_perc = attr[3];

    srand(time(NULL) - getpid()%1000 * 10000);
    passenger = (Passenger *) malloc(sizeof(Passenger));

    passenger->pid = getpid();

    // generate random validity based on percentages
    // add it later to the data file
    // srand(time(NULL));
    int temp = rand() % 100;
    passenger->valid = temp <= valid_perc ? true : false; // to be handled

    // generate random nationalities based on percentages
    //srand(time(NULL));
    temp = rand() % 100; // 30
    if (temp < pal_perc) passenger->nat = 'P';
    else if (temp > pal_perc && temp < 100 - jor_perc) passenger->nat = 'J';
    else passenger->nat = 'F';

    // generate random impatient time for passenger from 100 - 199
    //srand(time(NULL));
    temp = rand() % MAX_PASSENGER_WAIT_TIME + MIN_PASSENGER_WAIT_TIME;
    passenger->max_wait = temp;

}

/*
 * Sends the passenger struct using a queue mid to the arrival process
 * */
void send_passenger_to_arrival(int mid) {

    PASSENGER_MSG_QUEUE msg;

    msg.mtext[0] = *passenger;
    msg.mtype = PASSENGER_INFO_MTYPE_QUEUE;

    if (msgsnd(mid, &msg, sizeof(msg.mtext), 0) == -1) {
        perror("Send passenger to arrival by msq");
        exit(-3);
    }

}

/*
 * Sends the passenger struct using a queue mid to the arrival process
 * */
void send_passenger_to_officer(int mid, char nat) {

    PASSENGER_MSG_QUEUE msg;

    msg.mtext[0] = *passenger;

    if (nat == 'P') msg.mtype = PAL_PASSENGER_MTYPE_QUEUE;
    else if (nat == 'J') msg.mtype = JOR_PASSENGER_MTYPE_QUEUE;
    else msg.mtype = FOG_PASSENGER_MTYPE_QUEUE;

    if (msgsnd(mid, &msg, sizeof(msg.mtext), 0) == -1) {
        perror("Send passenger to officer by msq");
        exit(-3);
    }

}

/*
 * Reads the message queue to get the message struct of passenger attributes
 * */
INT_MSG_QUEUE receive_passenger_attr(int mid) {

    INT_MSG_QUEUE msg;

    if (msgrcv(mid, &msg, sizeof(msg.mtext), PASSENGER_ATTR_MTYPE_QUEUE, 0) == -1) {
        perror("Receive passenger attr by msq");
        exit(-3);
    }

    return msg;
}

bool listen_from_officer(int mid) {

    VALID_MSG_QUEUE msg;
    int pid = getpid();

    if (msgrcv(mid, &msg, sizeof(msg.mtext), pid, 0) == -1) {
        perror("Receive passenger validity from officer by msq");
        exit(-3);
    }

    return msg.mtext[0];
}

void send_to_hall(int mid) {

    PASSENGER_MSG_QUEUE msg;

    msg.mtype = PASSENGER_HALL_MSG_QUEUE;
    msg.mtext[0] = *passenger;

    if (msgsnd(mid, &msg, sizeof(msg.mtext), 0) == -1) {
        perror("Send passenger to hall by msq");
        exit(-3);
    }

    printf(GREEN"\nPassenger %d is valid and sent succesfully to hall\n"RESET,passenger->pid);
    fflush(stdout);

}

/*
 * This function increments the value of the variables related to the simualtion
 * */
void operate_critical_section(int semid, int* shmptr, int index) {

    acquire.sem_num = index;
    if(semop(semid,&acquire,1) == -1) {
        perror("error in acquiring sem - passenger ");
        fflush(stderr);
        exit(-11);
    }

    shmptr[index] = shmptr[index] + 1 ; // incrementing pg or pu or pd based on index came from different functions.
   // printf("\n\n now in passenger %d critical section\nat index %d with val %d\n",passenger->pid,index,shmptr[index]);
   // fflush(stdout);

    release.sem_num = index;
    if(semop(semid,&release,1) == -1) {
        perror("error in releasing sem");
        exit(-12);
    }

}

void validity_process(bool valid, int mid, int* shmptr, int semid) {

    alarm(0);

    if (!valid) {

        printf(MAGENTA"passenger %d IS NOT valid\n"RESET, passenger->pid);
        fflush(stdout);

        operate_critical_section(semid,shmptr,PD_INDEX);

        exit(0);
    } else {

        // send the vaild passenger to hall
        send_to_hall(mid);

        operate_critical_section(semid,shmptr,PG_INDEX);
    }

}

int sem_temp, *shmptr_tmp;

void alarm_catcher(int id) {
    operate_critical_section(sem_temp,shmptr_tmp,PI_INDEX);
    printf(MAGENTA"Passenger %d LEFT due to imptiency \n"RESET, passenger->pid);
    fflush(stdout);
    exit(0);
}

void signal_catcher(int id) {
    exit(0);
}


int main(int argc, char *argv[]) {

    bool valid;

    signal_sensitive();

    int mid = open_queue();
    set_passenger_attr(receive_passenger_attr(mid).mtext);


    //send_passenger_to_arrival(mid);
    send_passenger_to_officer(mid, passenger->nat);

    int shmid = open_shmem();
    void * shmptr = (int *) shmem_attach(shmid);
    int semid_gv = open_semaphore();

    sem_temp = semid_gv; 
    shmptr_tmp = shmptr;


    alarm(passenger->max_wait); // start the alarm

    valid = listen_from_officer(mid);

    validity_process(valid,mid,shmptr,semid_gv);

    while (1);

    return 0;

}