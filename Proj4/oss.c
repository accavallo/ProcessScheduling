//
//  main.c
//  Proj4
//
//  Created by Tony on 10/25/16.
//  Copyright © 2016 Anthony Cavallo. All rights reserved.
//

#include "Proj4.h"

int main(int argc, const char * argv[]) {
    signal(SIGSEGV, faultHandler);
    signal(SIGINT, interruptHandler);
    
    int option, index, time_increment = 1400;
    char *fileName = "logfile.txt";
    
    while ((option = getopt(argc, (char **)argv, "hm:l:")) != -1) {
        switch (option) {
            case 'h':
                printHelp();
                break;
            case 'm':
                if(isdigit(*optarg) && atoi(optarg) > 0)
                    time_increment = atoi(optarg);
                else
                    fprintf(stderr, "Expected positive integer, found %s instead\n", optarg);
                break;
            case 'l':
                fileName = optarg;
                break;
            case '?':
                if (optopt == 'm' || optopt == 'l')
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf(stderr, "Unknown option '-%c'.\n", optopt);
                else
                    fprintf(stderr, "Unknown option character '%s'.\n", optarg);
                break;
            default:
                printHelp();
                break;
        }
    }
    
    for (index = optind; index < argc; index++)
        printf("Non-option argument %s.\n", argv[index]);

    
    
    printf("Suuuup....\n");
    int processCount, status;
    
    printf("pid = %i\n", getpid());
    
    /* Set up shared memory */
    /***** NOTE: *****/
    /* seconds_memory ONLY NEEDS TO BE sizeof(int) * 2! block_memory needs only be sizeof(PCB) * 18 */
    if ((seconds_memory = shmget(TIMEKEY, sizeof(int) * 2, IPC_CREAT | 0777)) == -1) {
        printf("Error: Failed to create shared memory for seconds. Exiting program...\n");
        perror("OSS shmcreat");
        exit(1);
    }
    
    if ((seconds = shmat(seconds_memory, NULL, 0)) == (unsigned *)(-1)) {
        printf("Error: Failed to attach to shared memory for seconds. Exiting program...\n");
        perror("OSS shmat");
        exit(1);
    }
    
    nano_seconds = seconds + 1;
    bit_vector = seconds + 2;
    
    if ((block_memory = shmget(PCBKEY, sizeof(PCB) * 18, IPC_CREAT | 0777)) == -1) {
        printf("Error: Failed to create shared memory for Process Control Blocks. Exiting program...\n");
        perror("OSS PCB shmcreat");
        exit(1);
    }
    
    if ((blockArray = shmat(block_memory, NULL, 0)) < 0) {
        printf("Error: Failed to attach to shared memory for PCB's. Exiting program...\n");
        perror("OSS PCB shmat");
        exit(1);
    }

    processCount = 0;
    char procID[5];
    int bitArray[18] = {0}, totalProcesses = 0;
    pid_t pid, wpid;
    
    srand((unsigned)time(0));
    alarm(10);
    signal(SIGALRM, alarmHandler);
    while (*seconds < 50 || totalProcesses == 100) {
        *nano_seconds += (1 + rand() % time_increment);
        if (*nano_seconds >= 1000000000) {
            (*seconds)++;
            *nano_seconds %= 1000000000;
        }
        
        if (processCount < 18) {
            for (index = 0; index < 18; index++)
                if(bitArray[index] == 0) {
                    bitArray[index] = 1;
                    break;
                }
            
            pid = fork();
            processCount++;
            totalProcesses++;
            sprintf(procID, "%i", processCount);
            
            if (pid == 0) {
                newProcess(processCount, index);        //Not necessary, but it cleans up my main.
                execl("./user", procID, (char *)NULL);
                exit(EXIT_FAILURE);
            }
        }
        int processID = waitpid(0, NULL, WNOHANG);
        if (processID > 0) {
            printf("Process %i finished execution.\n", processID);
            processCount--;
            for (index = 0; index < 18; index++)
                if (blockArray[index].pid == processID)
                    bitArray[index] = 0;
        }
    }
    
    sleep(3);
    while ((wpid = wait(&status)) > 0);
    
    printf("Time is %i.%i\n", *seconds, *nano_seconds);
    printf("Finished waiting\n");

    detachEverything();
    return 0;
}

void newProcess(int processCount, int index) {
    blockArray[processCount-1].timeCreated = *seconds * 1000000000 + *nano_seconds;
    blockArray[processCount-1].runTime = 0;
    blockArray[processCount-1].timeElapsed = 0;
    blockArray[processCount-1].burstTime = 0;
    blockArray[processCount-1].timeInSystem = 0;
    blockArray[processCount-1].priority = rand() % 4;
    blockArray[processCount-1].isValid = 1;
    blockArray[processCount-1].procID = index;
    blockArray[processCount-1].pid = getpid();
}

void printHelp() {
    printf("-h displays this help message\n");
    printf("-l filename sets the filename to be used for the log file\n");
    printf("-m y sets the Integer argument 'y' to the variable 'm' for the amount the \"clock\"\n   will potentially increase by every time the loop iterates. (Increase time is a modulo of this number)\n");
//    printf("-s x sets the Integer argument 'x' to the variable 's' for the number of user\n   processes spawned. By default x is set to 5.\n");
//    printf("-t z sets the Integer argument 'z' to the variable 't' for the amount of time,\n   in seconds, when oss will terminate itself. By default z is set to 20.\n");
}

void faultHandler() {
    printf("\nDO YOU WANT TO GO TO WAR BALAKE?! GO SEE PRINCIPLE O'SHAUGHNESSY!\n");
    pid_t pid = getpid(), wpid, groupID = getpgrp(); int status = 0;
    killpg(groupID, SIGINT);
    while ((wpid = wait(&status)) > 0); //Wait for all children to exit before continuing execution.
    
    detachEverything();
    fprintf(stderr, "OSS process %d dying due to a segmentation fault.\n", pid);
    exit(0);
}

void interruptHandler() {
    printf("\nGoodbye, so soon... until... we meet... again...\n");
    pid_t pid = getpid(), groupID = getpgrp(), wpid; int status = 0;
    killpg(groupID, SIGINT);
    while ((wpid = wait(&status)) > 0);	//Wait for all children to exit before continuing execution.
    
    detachEverything();
    fprintf(stderr, "OSS process %d dying due to an interrupt.\n", pid);
    exit(0);
}

void otherInterrupt() {
    printf("I see that we have learned nothing about finishing on time...\n");
    pid_t groupID = getpgrp();
    killpg(groupID, SIGINT);
}

void alarmHandler() {
    printf("\nGood job, you let time run out...\n");
    pid_t pid = getpid(), groupID = getpgrp(), wpid; int status = 0;
    killpg(groupID, SIGALRM);
    while ((wpid = wait(&status)) > 0);
    
    detachEverything();
    fprintf(stderr, "You get NO pudding because your processes didn't finish on time.\nOSS process %d dying due to time being up.\n", pid);
    exit(0);
}

void detachEverything() {
    /* Detach from and remove shared memory */
    shmdt(seconds);
    shmdt(nano_seconds);
    shmdt(bit_vector);
    shmctl(seconds_memory, IPC_RMID, NULL);     //Remove shared memory
    
    shmdt(blockArray);
    shmctl(block_memory, IPC_RMID, NULL);       //Remove the block memory
}
