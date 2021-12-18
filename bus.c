
#include "local.h"

int Tbi_min,Tbi_max;
const int bus_max_wait = 200; 

/*
 * Generates the key and then opens the message queue created before using that key
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

BUS_MSG_QUEUE receive_attr (int mid) {

    BUS_MSG_QUEUE msg;

    if (msgrcv(mid, &msg, sizeof(msg.mtext), BUS_ATTR_MTYPE_QUEUE, 0) == -1) {
        perror("Reading from arrival queue");
        exit(-3);
    }

    return msg;

}

void set_attr(BUS_MSG_QUEUE msg) {

    Tbi_min = msg.mtext[0];
    Tbi_max = msg.mtext[1];

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


int * shmem_attach(int shmid)
{
    void * shmptr;
    if ((shmptr = shmat(shmid,0,0)) == (int*) -1) {
        perror("attaching to an address");
        exit(-5);
    }
    return shmptr;
}

int open_semaphore () {

    int key,semid;

    /*
     * Get the key to open the message queue
     * */
    if ((key = ftok(".", SEED_SEM)) == -1) {
        perror("Public Semaphore open");
        exit(-1);
    }
    if ((semid = semget(key,4,0)) == -1) {
        perror("semaphore error");
        exit(-6);
    }
    return semid;
}

PASSENGER_MSG_QUEUE receive_passenger(int mid) {

    PASSENGER_MSG_QUEUE msg;

    if (msgrcv(mid, &msg, sizeof(msg.mtext), BUS_PASSENGER_MTYPE_QUEUE, 0) == -1) {
        perror("Reading from arrival queue");
        exit(-3);
    }

    return msg;
}

void welcome_to_jordan () {
    alarm(0);
    printf(BLUE"Bus %d left\n"RESET,getpid());
    fflush(stdout);
    printf(GREEN"WELCOME "RED"TO "CYAN"JORDAN\n"RESET);
    fflush(stdout);
    srand(time(NULL) - getpid()*1000);
    sleep(rand()%Tbi_max + Tbi_min);

}

/*
 * Todo: msgrcv is stuck and the semaphore was stuck with it
 * */

void operate_critical_section (int semid, int mid, int* shmptr ) {

    Passenger passenger = receive_passenger(mid).mtext[0];

 kill(passenger.pid,SIGUSR1);

    acquire.sem_num = CN_INDEX;
    if(semop(semid,&acquire,1) == -1) {
        perror("acquiring sem in bus");
        exit(-11);
    }

    // start of  critical section
    shmptr[CN_INDEX] = shmptr[CN_INDEX] - 1 ;
    printf(GREEN"number of passengers in hall %d\npassenger %d headed to bus %d peacefully\n"RESET,shmptr[CN_INDEX],passenger.pid,getpid());
    fflush(stdout);
    // end of critical section

    release.sem_num = CN_INDEX;
    if(semop(semid,&release,1) == -1) {
        perror("releasing sem in bus");
        exit(-12);
    }
   
}
void signal_alarm (int id) {
    welcome_to_jordan();
}

void signal_catcher(int id) {
    exit(0);
}

void signal_sensitive() {
    if (sigset(SIGUSR1, signal_catcher) == -1) {
        perror("Cannot set SIGUSR1");
        exit(SIGUSR1);
    }
    if (sigset(SIGALRM, signal_alarm) == -1) {
        perror("Cannot set SIGALRM");
        exit(SIGALRM);
    }    
}


int main(int argc, char *argv[]) {

    int mid,semid;

    int shmid;
    int *shmptr;

    signal_sensitive();

    mid = open_queue();

    set_attr(receive_attr(mid));

    // open shared memory
    shmid = open_shmem();
    shmptr=shmem_attach(shmid);

    semid=open_semaphore();

    while (1){
        alarm(bus_max_wait);
        for (int i = 0; i < BUS_CAPACITY ; ++i) {
            operate_critical_section(semid, mid, shmptr);
        }
        welcome_to_jordan();
    }

}