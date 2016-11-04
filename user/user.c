//  user.c
//  user
//
//  Created by Tony on 10/25/16.
//  Copyright © 2016 Anthony Cavallo. All rights reserved.
//

#include "Proj4.h"

int main(int argc, const char * argv[]) {
    
    /****** NOTE: ******
     argv[0] is the child number given by oss. THIS IS 0-BASED!!
     argv[1] is
     argv[2] is 
     argv[3] is
     argv[4] is
     argv[5] is
     *******************/
    
    const int ID = atoi(argv[0]);
        
    /* Attach to shared memory for time */
    seconds_memory = shmget(TIMEKEY, sizeof(int) * 3, 0777);
    if (seconds_memory == -1) {
        printf("User %i failed to access time memory. Exiting...\n", ID+1);
        exit(1);
    }
    
    if ((seconds = shmat(seconds_memory, NULL, 0)) == (unsigned *)-1) {
        printf("User %i failed to attach to time memory. Exiting...\n", ID+1);
        exit(1);
    }
    
    nano_seconds = seconds + 1;
    quantum = seconds + 2;
    
    /* Attach to the Process Control Block array */
    block_memory = shmget(PCBKEY, sizeof(PCB) * 18, 0777);
    if (block_memory == -1) {
        printf("User %i failed to access block memory. Exiting...\n", ID+1);
    }
    
    if ((blockArray = shmat(block_memory, NULL, 0)) < 0) {
        printf("User %i failed to attach to block memory. Exiting...\n", ID+1);
    }
    
//    printf("blockArray[%i].pid: %i\n", ID, blockArray[ID].pid);
    
//    printf("%i's creation time is %llu with a priority of %i\n", blockArray[ID].procID, blockArray[ID].timeCreated, blockArray[ID].priority);
    printf("Process %i has a runtime of %llu\n", ID+1, blockArray[ID].runTime);
    /* 50000000 == 50 milliseconds */
    /* Need to set how long the program will run for */
    /* Need to change the process' validity back to 0 if it hasn't finished so it will wait its turn. */
    srand((unsigned)time(0));
    //TODO: Determine whether the process will finish it's time quantum
    while (blockArray[ID].timeElapsed <= blockArray[ID].runTime || blockArray[ID].timeElapsed <= 50000000) {
        /* Make sure the process is valid, otherwise put it into a ready state. */
        if (blockArray[ID].isValid) {
            /* Check if the process will run for it's entire quantum. */
            if (rand() % 2) {
                /* The process uses the entire quantum in this case... */
                blockArray[ID].timeElapsed += *quantum;
                blockArray[ID].burstTime += *quantum;
                *quantum = 0;
                /* The process finished it's quantum, therefore it moves down a queue. */
                if (blockArray[ID].priority != 0 && blockArray[ID].priority != 3)
                    blockArray[ID].priority--;
            } else {
                /* ...and only uses a part of it in this case. */
                int time_slice = rand() % *quantum;
                blockArray[ID].timeElapsed += time_slice;
                *quantum -= time_slice;
            }
        } else {
            printf("Process %i moving to inactive...\n", blockArray[ID].procID);
            readyStateWait(ID);
        }
    }
    
//    printf("Process %i has waited for %llu nanoseconds\n", ID + 1, blockArray[ID].timeElapsed);
    blockArray[ID].isValid = 0;
    blockArray[ID].didFinish = 1;
    sleep(1);
    detachEverything();
    return 0;
}

void readyStateWait(int ID) {
    
    blockArray[ID].burstTime = 0;
    while (1) {
        if (blockArray[ID].isValid) {
            printf("Process %i became valid!\n", blockArray[ID].procID);
            break;
        }
//        sleep(1);
    }
}

void faultHandler() {
    detachEverything();
    
    pid_t pid = getpid();
    printf("\nYOU DONE MESSED UP A-A-RON! YOU CREATED A SEGMENTATION FAULT!\n");
    fprintf(stderr, "USER process %d dying due to a segmentation fault.\n", pid);
    
    exit(0);
}

void interruptHandler() {
    detachEverything();
    
    pid_t pid = getpid();
    printf("Uh, oh spaghetti-o's! USER %d got an interrupt boss!\n", pid);
    fprintf(stderr, "USER process %d dying from an interrupt\n", pid);
    
    exit(0);
}

void alarmHandler() {
    detachEverything();
    
    pid_t pid = getpid();
    printf("Time waits for no process... that means you number %d!\n", pid);
    fprintf(stderr, "USER process %d dying due to time being up.\n", pid);
    
    exit(0);
}

void detachEverything() {
    shmdt(seconds);
    shmdt(nano_seconds);
    shmdt(quantum);
    shmdt(blockArray);
}
