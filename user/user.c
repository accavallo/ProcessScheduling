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
    seconds_memory = shmget(TIMEKEY, sizeof(int) * 2, 0777);
    if (seconds_memory == -1) {
        printf("User %i failed to access time memory. Exiting...\n", ID);
        exit(1);
    }
    
    if ((seconds = shmat(seconds_memory, NULL, 0)) == (unsigned *)-1) {
        printf("User %i failed to attach to time memory. Exiting...\n", ID);
        exit(1);
    }
    
    nano_seconds = seconds + 1;
    
    /* Attach to the Process Control Block array */
    block_memory = shmget(PCBKEY, sizeof(PCB) * 18, 0777);
    if (block_memory == -1) {
        printf("User %i failed to access block memory. Exiting...\n", ID);
    }
    
    if ((blockArray = shmat(block_memory, NULL, 0)) < 0) {
        printf("User %i failed to attach to block memory. Exiting...\n", ID);
    }
    
    long long unsigned quantum = 0;
//    while (1) {
//        if (nextInQueue) {
//            
//        }
//        sleep(1);
//    }
//    printf("blockArray[%i].pid: %i\n", ID, blockArray[ID].pid);
    
    printf("%i's creation time is %llu with a priority of %i\n", ID+1, blockArray[ID].timeCreated, blockArray[ID].priority);
    /* 50000000 == 50 milliseconds */
    /* Need to set how long the program will run for */
    srand((unsigned)time(0));
    long long currentTime = *seconds * 1000000000 + *nano_seconds;
    long long previousTime = *seconds * 1000000000 + *nano_seconds;
    while (blockArray[ID].timeElapsed <= blockArray[ID].runTime || blockArray[ID].timeElapsed <= 50000000) {
//        while (blockArray[ID].timeElapsed < 50000000) {
        if (blockArray[ID].isValid) {
            currentTime = *seconds * 1000000000 + *nano_seconds;
            blockArray[ID].timeElapsed += currentTime - previousTime;
            previousTime = currentTime;
        } else {
            readyStateWait(ID);
        }
//        }
        blockArray[ID].timeElapsed += rand() % 500000;
    }
    
    printf("Process %i has waited for 50 milliseconds\n", ID);
    blockArray[ID].isValid = 0;
//    blockArray[ID].didFinish = 1;
    sleep(1);
    detachEverything();
    return 0;
}

void readyStateWait(int ID) {
    while (1) {
        if (blockArray[ID].isValid) {
            printf("Process %i became valid!\n", ID+1);
            break;
        }
        sleep(1);
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
    shmdt(blockArray);
}
