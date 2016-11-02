 //  Proj4.h
//  Proj4
//
//  Created by Tony on 10/25/16.
//  Copyright © 2016 Anthony Cavallo. All rights reserved.
//

#ifndef Proj4_h
#define Proj4_h

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <errno.h>

#define TIMEKEY 18137644
#define PCBKEY 44673181

typedef struct process_control_block {
    long long timeCreated;      //Time the process was created
    long long runTime;          //How long process will run for
    long long timeElapsed;      //How long process has run
    long long burstTime;        //Time process used during last burst
    long long timeInSystem;     //How long the process has actually been in the system.
//    int didFinish;              //Determines whether the process is completely finished.
    int priority;               //Priority of the process
    int isValid;                //Is process still valid
    int procID;                 //Process ID the program sets
    pid_t    pid;               //ACTUAL process ID
} PCB;

typedef struct process_queue {
    int location;
    PCB *process;
    struct process_queue *next_node;
}procq_t;

int seconds_memory;
int block_memory;

/* Items in shared memory */
unsigned int *seconds;
unsigned int *nano_seconds;
unsigned int *bit_vector;
PCB *blockArray;

void faultHandler();
void interruptHandler();
void otherInterrupt();
void alarmHandler();
void detachEverything();

/* OSS specific methods */
void newProcess(int, int);
void removeProcess(int);
void addToQueue(int, int);
//void addToQueue(int, PCB *, int);
void printQueue(procq_t *);
void printHelp();

/* USER specific methods */
void readyStateWait();


#endif /* Proj4_h */
