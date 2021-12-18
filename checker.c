#include "local.h"

int stop_flag = 0;
/*
 * Opens a public message queue
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

/*
 * Removes the public message queue
 * */
void remove_queue(int mid) {
    struct msqid_ds msg;
    if (msgctl(mid,IPC_RMID,&msg) == -1) {
        perror("Removing arrival queue");
        exit(-1);
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

void signal_for_all() {
    kill(getppid(),SIGUSR2);  // makes parent flush everything
}

void * shmem_attach(int shmid)
{
    void * shmptr;
    if ((shmptr = shmat(shmid,0,0)) == (int *) -1) {
        perror("attaching to an address");
        exit(-5);
    }
    return shmptr;
}

int open_semaphore_gv() {

    int key,semid;

    /*
     * Get the key to open the message queue
     * */
    if ((key = ftok(".", SEED_SEM)) == -1) {
        perror("Public message queue key generation");
        exit(-1);
    }
    if ((semid = semget(key,4,0)) == -1) { // create semaphore// nsems = 1
        perror("semaphore error");
        exit(-6);
    }

    return semid;
}

void wait_for_mq (int mid,int * shmptr) {

    struct msqid_ds msq_ds;

    while (1) {
        if (msgctl(mid, IPC_STAT, &msq_ds) != 0) {
            perror("msgctl in checker");
            exit(-2);
        }

        if (msq_ds.msg_qnum == 0) { // not convincing 
            printf("\nTRIP IS OVER - msq is empty\n");
            fflush(stdout);
            signal_for_all();
            exit(0);
        }
    }

}


int check_status (int semid, const int * shmptr,int mid) {

    int flag = 0;

    for (int i = 0; i < 3; ++i) {
        acquire.sem_num = i;
        if (semop(semid, &acquire, 1) == -1) {
            perror("error in acquiring sem checker");
            exit(-11);
        }
    }

    if ( shmptr[PD_INDEX] == PASSENGERS_DENIED || shmptr[PG_INDEX] == PASSENGERS_GRANTED || shmptr[PI_INDEX] == PASSENGERS_IMPATIENT || shmptr[TOT_NUM_INDEX] <= 0 ) {
        kill(getppid(),SIGUSR1);  // make parent stops forking passengers
        printf("PG PD PU HAVE BEEN ACTIVATED AND A SIGNAL SENT TO PARENT\n\n "
               "PG = %d, PD = %d,PU = %d",shmptr[PG_INDEX],shmptr[PD_INDEX],shmptr[PI_INDEX]);
        fflush(stdout);
        flag = 1;
    }


    for (int i = 0; i < 3; ++i) {
        release.sem_num = i;
        if(semop(semid,&release,1) == -1) {
            perror("error in releasing sem");
            exit(-12);
        }
    }
    
    return flag;

}


int main (int argc, char* argv[])
{

    int mid = open_queue();

    int shmid = open_shmem();
    int *shmptr = shmem_attach(shmid);

    int semid = open_semaphore_gv();

    while(!check_status(semid,shmptr,mid));
    
    wait_for_mq(mid,shmptr);

    return 0;
} 