#ifndef __LOCAL_H_
#define __LOCAL_H_

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <wait.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>
#include <sys/msg.h>

#define SEED_MSQ 'a'
#define SEED_SHM 'z'
#define SEED_SEM 'q'

#define SEEDsem 's'
#define SEEDsem2 'h'

#define MAX_BUF 50 // read file string
#define INPUT_LINES 14

#define MAX_PASSENGER_NUMBER_PER_TRIP 20

#define MAX_PASSENGER_TRIP_TIME 30
#define MIN_PASSENGER_TRIP_TIME 5

#define MAX_PASSENGER_WAIT_TIME 40
#define MIN_PASSENGER_WAIT_TIME 30

#define MAX_OFFICER_PROCESS_TIME 10
#define MIN_OFFICER_PROCESS_TIME 5


#define PASSENGERS_GRANTED 40
#define PASSENGERS_DENIED 30
#define PASSENGERS_IMPATIENT 30 
#define SHMEM_SIZE 1024


#define BUS_CAPACITY 10

/*
 * THIS COMMENT WAS REQUESTED BY BATOOL
 * ITS THE VARIABLES INDICES IN THE SHARED MEMORY
 * */
#define PG_INDEX       0
#define PD_INDEX       1
#define PI_INDEX       2
#define CN_INDEX       3 // current number 
#define TOT_NUM_INDEX  4 

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

struct sembuf acquire = {0, -1, SEM_UNDO},
        release = {0, 1, SEM_UNDO};


typedef struct shmem {
    char buffer[MAX_BUF][10];
    int head, tail;
} MEMORY;


typedef struct passenger {
    int pid;
    char nat;
    bool valid; // passport check
    int max_wait; // impatient time
    int arrival_time; // initalized in arraival
} Passenger;

typedef struct msg {
    long mtype;
    Passenger mtext[1];
} PASSENGER_MSG_QUEUE;

typedef struct msgbuf {
    long mtype;
    int mtext[4];
} INT_MSG_QUEUE;

typedef struct {
    long mtype;
    char mtext[1];
} OFF_ATTR_MSG_QUEUE;

typedef struct {
    long mtype;
    bool mtext[1];
} VALID_MSG_QUEUE;

typedef struct {
    long mtype;
    int mtext[2];
} HALL_MSG_QUEUE, BUS_MSG_QUEUE;

#define PASSENGER_ATTR_MTYPE_QUEUE 1
#define PASSENGER_INFO_MTYPE_QUEUE 2

#define PAL_PASSENGER_MTYPE_QUEUE 970
#define JOR_PASSENGER_MTYPE_QUEUE 962
#define FOG_PASSENGER_MTYPE_QUEUE 555

#define HALL_ATTR_MTYPE_QUEUE 3
#define PASSENGER_HALL_MSG_QUEUE 4
#define BUS_ATTR_MTYPE_QUEUE 5
#define BUS_PASSENGER_MTYPE_QUEUE 6


#define RED     "\x1b[31m"
#define GREEN   "\x1b[32m"
#define YELLOW  "\x1b[33m"
#define BLUE    "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN    "\x1b[36m"
#define RESET   "\x1b[0m"



#endif
