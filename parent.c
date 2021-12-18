
/// ABDALLAH KHAROUF 1183328 & ALA AHMAD 1183339 & BATOOL SULAIBI 1182072

#include "local.h"

/*
 * Simulation Attributes
 * */
int
        passenger_no,
        officer_no, pal_off, jor_off, fog_off,
        pal_perc, jor_perc, fog_perc,
        bus_no,
        Tbi_min, Tbi_max,
        H_max, H_min,
        passenger_granted = 0, passenger_denied = 0, passenger_imp = 0, valid_perc;

/*
 * child processes' pids
 * */
int hall_process, checker_process , * officer_processes, *bus_processes , *passenger_processes;

/*
 * ID of the public message queue
 * */
int mid;

/*
 * Indicates the status of the simulation
 * */
int simulation_flag = 1;

/*
 * Parses a 2D string and sets the global simulation attributes
 * */
void set_global_attr(char buffer[INPUT_LINES][MAX_BUF]) {

    int stats[INPUT_LINES];

    for (int i = 0; i < INPUT_LINES; ++i) {
        char *token;
        token = strtok(buffer[i], " ");
        token = strtok(NULL, " ");
        stats[i] = atoi(token);
    }

    // set attr
    passenger_no = stats[0];

    officer_no = stats[1];
    pal_off = stats[2];
    jor_off = stats[3];
    fog_off = stats[4];

    pal_perc = stats[5];
    jor_perc = stats[6];
    fog_perc = stats[7];

    bus_no = stats[8];

    Tbi_max = stats[9];
    Tbi_min = stats[10];

    H_max = stats[11];
    H_min = stats[12];
    valid_perc = stats[13];

}

/*
 * Reads the input file, puts values in 2D String
 * and then calls set_global_attr to update global simulation variables
 * */
void read_file() {

    char buffer[INPUT_LINES][MAX_BUF];

    FILE *data = NULL;
    char filename[MAX_BUF] = "data.txt";

    data = fopen(filename, "r");

    if (data == NULL) {
        perror("opening data.txt");
        exit(-1);
    }

    // attempt to read the next line and store
    // the value in the "number" variable
    for (int i = 0; i < INPUT_LINES; ++i) {
        while (fgets(buffer[i], sizeof(buffer[i]), data) != NULL) {
            ++i;
        }
    }

    set_global_attr(buffer);
}

/*
 * Creates a public message queue
 * */
int create_queue() {

    int key,mq_id;

    if ((key = ftok(".", SEED_MSQ)) == -1) {
        perror("Public message queue key generation");
        exit(-1);
    }

    if ((mq_id = msgget(key, IPC_CREAT | 0666)) == -1) { // this mid should be sent to parent process to be sent to passengers
        perror("Public message queue creation");
        exit(-2);
    }
    return mq_id;
}

/*
 * Removes the public message queue
 * */
void remove_queue() {
    struct msqid_ds msg;
    if (msgctl(mid,IPC_RMID,&msg) == -1) {
        perror("Removing arrival queue");
        exit(-1);
    }
}

/*TODO
 * use one semaphores instead of two
 * */

/*
 * Creates the public semaphore
 * */
int create_semaphore (union semun arg) {

    int key,semid;

    /*
     * Get the key to open the semaphore
     * */
    if ((key = ftok(".", SEED_SEM)) == -1) {
        perror("Public semaphore generation");
        exit(-1);
    }
    if ((semid = semget(key,4,IPC_CREAT | 0666)) == -1) { // create semaphore// nsems = 1
        perror("semget error");
        exit(-6);
    }
    if (semctl(semid,0,SETALL,arg) == -1) {
        perror("semctl error");
        exit(-7);
    }
    return semid;
}

/*
 * Creates the public shared memory
 * */
int create_shmem () {

    int key,shmid;

    if ((key = ftok(".", SEED_SHM)) == -1) {
        perror("Public key generation");
        exit(-1);
    }

    if ((shmid = shmget(key,SHMEM_SIZE, IPC_CREAT | 0644)) == -1) { // this mid should be sent to parent process to be sent to passengers
        perror("shared memory opening");
        exit(-2);
    }
    return shmid;

}

/*
 * Attaches the public shared memory
 * */
void * shmem_attach(int shmid)
{
    void * shmptr;
    if ((shmptr = shmat(shmid,0,0)) == (int *) -1) {
        perror("attaching to an address");
        exit(-5);
    }
    return shmptr;
}

/*
 * Sets global attributes to shared memory
 * */
int * attach_global_resources (int * shmptr) {

    shmptr[PG_INDEX] = passenger_granted;
    shmptr[PD_INDEX] = passenger_denied;
    shmptr[PI_INDEX] = passenger_imp;
    shmptr[CN_INDEX] = 0;

    // 
    shmptr[TOT_NUM_INDEX] = passenger_no; 


    return shmptr;
}

/*
 * Sends officer's attributes to each officer using a message queue
 * */
void send_officer_attr (int oid) {

    char nat;

    // determine nationality of each officer
    if (pal_off > 0) {
        nat = 'P';
        pal_off-- ;
    }
    else if (jor_off > 0) {
        nat = 'J';
        jor_off-- ;
    }
    else if (fog_off > 0) {
        nat = 'F';
        fog_off-- ;
    }
    else nat = 'U';

    OFF_ATTR_MSG_QUEUE msg ;

    msg.mtype = oid;
    msg.mtext[0] = nat;

    if( msgsnd(mid, &msg, sizeof(msg.mtext), 0) == -1 ) {
        perror("Reading from arrival queue");
        exit(-3);
    }

}

/*
 * Sends passenger's attributes to each passenger using a message queue
 * */
void send_passenger_attr () {

    INT_MSG_QUEUE msg;

    msg.mtype = PASSENGER_ATTR_MTYPE_QUEUE;
    msg.mtext[0] = pal_perc;
    msg.mtext[1] = jor_perc;
    msg.mtext[2] = fog_perc;
    msg.mtext[3] = valid_perc;

    if( msgsnd(mid, &msg, sizeof(msg.mtext), 0) == -1 ) {
        perror("Reading from arrival queue");
        exit(-3);
    }
}

/*
 * Sends hall's attributes to hall using a message queue
 * */
void send_hall_attr () {

    HALL_MSG_QUEUE msg ;

    msg.mtype = HALL_ATTR_MTYPE_QUEUE;
    msg.mtext[0] = H_min;
    msg.mtext[1] = H_max;

    if( msgsnd(mid, &msg, sizeof(msg.mtext), 0) == -1 ) {
        perror("Reading from arrival queue");
        exit(-3);
    }

}

/*
 * Sends bus's attributes to each bus using a message queue
 * */
void send_bus_attr () {

    BUS_MSG_QUEUE msg ;

    msg.mtype = BUS_ATTR_MTYPE_QUEUE;
    msg.mtext[0] = Tbi_min;
    msg.mtext[1] = Tbi_max;

    if( msgsnd(mid, &msg, sizeof(msg.mtext), 0) == -1 ) {
        perror("Reading from arrival queue");
        exit(-3);
    }

}

/*
 * Forking and executing different processes needed in the system
 * */
void implement_stages() {

    // forking officers
    for (int i = 0; i < officer_no; i++) {
        switch (officer_processes[i] = fork()) {
            case -1:
                perror("Officer Forking");
                exit(-1);
            case 0:  // child process
                execlp("./officer.o", "officer.o", (void *) NULL);
                break;
            default:  // parent
                send_officer_attr(officer_processes[i]);
                break;
        }
    }

    //forking hall
    switch (hall_process = fork()) {
        case -1:
            perror("Hall Forking");
            exit(-1);
        case 0:  // child process
            execlp("./hall.o", "hall.o", (void *) NULL);
            break;
        default:  // parent
            send_hall_attr();
            break;
    }

    // forking buses
    for (int i = 0; i < bus_no; i++) {
        switch (bus_processes[i] = fork()) {
            case -1:
                perror("Buss Forking");
                exit(-1);
            case 0:  // child process
                execlp("./bus.o", "bus.o", (void *) NULL);
                break;
            default:  // parent
                send_bus_attr();
                break;
        }
    }

    // forking checker
    switch (checker_process=fork()) {
        case -1:
            perror("Check process fails\n\n");
            break;
        case 0: // child process
            if (execlp("./checker.o", "checker.o", (char *) NULL) == -1) {
                perror("execlp checker.o");
                exit(-2);
            }
            break;
        default:  // parent
            break;
    }
}


/*
 * Generates passengers randomly for each trip
 * and sends them their attributes by calling send_passenger_attr
 * */
void passengers_generation(int *shmptr) {

    int number_of_passengers_per_trip;

    time_t temp;
    srand(time(&temp));


    number_of_passengers_per_trip = rand() % MAX_PASSENGER_NUMBER_PER_TRIP + 1; // per trip -- random

    for (int i = 0; i < number_of_passengers_per_trip && passenger_no > 0 && simulation_flag; ++i, --passenger_no,--shmptr[TOT_NUM_INDEX]) {
        switch (passenger_processes[i] = fork()) {
            case -1:
                perror("Passenger Forking");
                exit(-1);
            case 0:  // child process
                if (execlp("./passenger.o", "passenger.o", (char *) NULL) == -1) {
                    perror("execlp passenger.o");
                    exit(-2);
                }
                break;
            default:  // parent
                send_passenger_attr();
                break;
        }
        printf(BLUE"Number of remainig passengers %d \n\n"RESET,shmptr[TOT_NUM_INDEX]);
    }


}

/*
 * Organize trips of passengers to be sent
 * */
void trip_organizer (int * shmptr) {

    srand(time(NULL) - getpid()%1000*10000);

    while (passenger_no > 0 && simulation_flag) {

        passengers_generation(shmptr);

        sleep(rand() % MAX_PASSENGER_TRIP_TIME + MIN_PASSENGER_TRIP_TIME);

    }

}

/*
 * SIGUSR2 catcher
 * */
void signal_catcher (int id) {
    simulation_flag = 0;
    printf(RED "\nNO PASSENGERS ARE ALLOWED TO ENTER THE SIMULATION\n"
           "\tCLOSED FOR THE DAY !!!\n"RESET); 
    fflush(stdout);
}

/*
 * SIGUSR1 catcher
 * */
void signal_catcher2 (int id) {

    pid_t wpid;
    int status = 0;

  //  for (int i = 0 ; i < j; i++) {if(waitpid(passenger_processes[i],0) == -1){perror("waitpid failed"); exit(-1);}}

for (int i = 0; i < officer_no; ++i) {
        kill(officer_processes[i], SIGUSR1);
}

    kill(hall_process,SIGUSR1);
    

    for (int i = 0; i < bus_no; ++i) {
        kill(bus_processes[i], SIGUSR1);
    }

    while ((wpid = wait(&status)) > 0);

    remove_queue();

    exit(0);

}

/*
 * Tell officers to stop working
 * */
void signal_catcher3 (int id) {
    for (int i = 0; i < officer_no; ++i)
        kill(officer_processes[i],SIGALRM);

    printf(RED"Hall is full\n"RESET);
    fflush(stdout)    ;
}

/*
 * Tell officers to start working
 * */
void signal_catcher4 (int id) {
    for (int i = 0; i < officer_no; ++i)
        kill(officer_processes[i],SIGBUS);
}

/*
 * Sets the sensitivity list of the parent process
 * */
void signal_sensitive() {

    if (sigset(SIGUSR1, signal_catcher) == -1) {
        perror("Cannot set SIGUSR1");
        exit(SIGINT);
    }
    if (sigset(SIGUSR2, signal_catcher2) == -1) {
        perror("Cannot set SIGUSR2");
        exit(SIGUSR2);
    }

    // hall - hamx hmin 
    if (sigset(SIGALRM, signal_catcher3) == -1) {
        perror("Cannot set SIGALRM");
        exit(SIGALRM);
    }
    if (sigset(SIGBUS, signal_catcher4) == -1) {
        perror("Cannot set SIGBUS");
        exit(SIGBUS);
    }
}


int main(int argc, char *argv[]) {

    int shmid, semid;
    void *shmptr;

    union semun arg_gv;
    arg_gv.array = (unsigned short []) {1,1,1,1};

    signal_sensitive();  // sets the sesitvity list

    read_file();  // read input file to initialize global variables

    officer_processes = malloc(sizeof(int) * officer_no);  // init the array of officers' pids
    bus_processes = malloc(sizeof(int) * bus_no);  // init the array of buses' pids
    passenger_processes = malloc(sizeof(int) * passenger_no);  // init the array of passengers pids

    mid = create_queue();  // create public queue

    shmid  = create_shmem();  // create public shared memory

    semid = create_semaphore(arg_gv);  // create public semaphore

    shmptr = (int*) shmem_attach(shmid);  // attach the shared memory to pointer *shmptr

    shmptr =  attach_global_resources(shmptr);  // attach the global variables to shared memory

    implement_stages();  // start forking processes

    trip_organizer(shmptr);  // organize sets of passengers

    while(1);

    return 0;
}