//  user.c
//  user
//
//  Created by Tony on 10/25/16.
//  Copyright © 2016 Anthony Cavallo. All rights reserved.
//

#include "Proj4.h"

int main(int argc, const char * argv[]) {
    
    /****** NOTE: ******
     argv[0] is the child number given by oss;
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
    sleep(1);
//    printf("blockArray[%i].pid: %i\n", ID, blockArray[ID-1].pid);
    
    printf("%i's creation time is %llu\n", ID, blockArray[ID-1].timeCreated);
    long long currentTime = *seconds * 1000000000 + *nano_seconds;
    while ((currentTime - blockArray[ID-1].timeCreated) < 5000000) {
        
        
        currentTime = *seconds * 1000000000 + *nano_seconds;
    }
    
    printf("Process %i has waited for 50 milliseconds\n", ID);
    sleep(1);
    detachEverything();
    return 0;
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
