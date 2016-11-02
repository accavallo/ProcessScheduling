//  oss.c
//  Proj4
//
//  Created by Tony on 10/25/16.
//  Copyright © 2016 Anthony Cavallo. All rights reserved.
//

#include "Proj4.h"

static procq_t *queue0 = NULL, *queue1 = NULL, *queue2 = NULL, *queue3 = NULL;

int main(int argc, const char * argv[]) {
    signal(SIGSEGV, faultHandler);
    signal(SIGINT, interruptHandler);
    
    int option, index, time_increment = 1000;
    char *fileName = "logfile.txt";
    
    while ((option = getopt(argc, (char **)argv, "hl:")) != -1) {
        switch (option) {
            case 'h':
                printHelp();
                break;
//            case 'm':
//                if(isdigit(*optarg) && atoi(optarg) > 0)
//                    time_increment = atoi(optarg);
//                else
//                    fprintf(stderr, "Expected positive integer, found %s instead\n", optarg);
//                break;
            case 'l':
                fileName = optarg;
                break;
            case '?':
//                if (optopt == 'm' || optopt == 'l')
                if (optopt == 'l')
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
    /******** NOTE: *********
     seconds_memory ONLY NEEDS TO BE sizeof(int) * 2! block_memory needs only be sizeof(PCB) * 18
     ************************/
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
    long long unsigned creationTime = 0, currentTime = 0, creationTimeSet = 0, processor_idle_time = 0;
    pid_t pid, wpid;
    
    srand((unsigned)time(0));
    alarm(30);
    signal(SIGALRM, alarmHandler);
    
//    for (index = 0; index < 5; index++) {
//        newProcess(index+1, index);
//        printf("blockArray[%i].priority: %i\n", index, blockArray[index].priority);
//    }
//    
//    printf("Queue 0\n");
//    printQueue(queue0);
//    printf("Queue 1\n");
//    printQueue(queue1);
//    printf("Queue 2\n");
//    printQueue(queue2);
//    printf("Queue 3\n");
//    printQueue(queue3);
    
    while (*seconds < 50 || totalProcesses == 100) {
        if (creationTimeSet == 0) {
            creationTime = *seconds * 1000000000 + *nano_seconds + (rand() % 2000000000);
            creationTimeSet = 1;
            printf("Creation time of process %i is set for %llu, %llu nanoseconds ahead.\n", processCount, creationTime, creationTime - currentTime);
        }
        currentTime = *seconds * 1000000000 + *nano_seconds;
        
//        printf("currentTime: %llu\n", currentTime);
        *nano_seconds += rand() % time_increment;
        if (*nano_seconds >= 1000000000) {
            (*seconds)++;
            *nano_seconds %= 1000000000;
        }
        
        /* No more than 18 processes at a time, and the current time needs to be after the creation time that was set "randomly". */
        if (processCount < 18 && creationTime <= currentTime) {
            for (index = 0; index < 18; index++)
                if(bitArray[index] == 0) {
                    bitArray[index] = 1;
                    break;
                }
            
            pid = fork();
            sprintf(procID, "%i", processCount);
            processCount++;
            totalProcesses++;
            creationTimeSet = 0;
            newProcess(processCount, index);        //Not necessary, but it cleans up my main.
            if (pid == 0) {
                
                execl("./user", procID, (char *)NULL);
                removeProcess(processCount-1);          //If the execl fails, this will essentially clear that process control block.
                exit(EXIT_FAILURE);
            }
//        } else {
//            printf("Waiting time is %i.%i\n", *seconds, *nano_seconds);
        }

        
        
        /* Do queue stuff here. Maybe some if else statements for the various queues. Maybe flip the isValid bit in the PCB and have the
           process wait until it is flipped */
        /* Ensure something exists in each queue, no else statements, before executing any code */
        
        /* Every node, after it's time slice, will move to the next node in the queue. */
        /* Do queue0 stuff */
        if (queue0) {
//            printf("Entering queue 0\n");
            blockArray[queue0->location].isValid = 1;
//            blockArray[queue1->location].isValid = 0;
//            blockArray[queue2->location].isValid = 0;
//            blockArray[queue3->location].isValid = 0;
            if (blockArray[queue0->location].isValid)
                addToQueue(0, queue0->location);
            
            queue0 = queue0->next_node;
        /* Do queue1 stuff */
        } else if (queue1) {
//            printf("Entering queue 1\n");
//            blockArray[queue0->location].isValid = 0;
            blockArray[queue1->location].isValid = 1;
//            blockArray[queue2->location].isValid = 0;
//            blockArray[queue3->location].isValid = 0;
            if (blockArray[queue1->location].isValid)
                addToQueue(1, queue1->location);
            
            queue1 = queue1->next_node;
        /* Do queue2 stuff */
        } else if (queue2) {
//            printf("Entering queue 2\n");
//            blockArray[queue0->location].isValid = 0;
//            blockArray[queue1->location].isValid = 0;
            blockArray[queue2->location].isValid = 1;
//            blockArray[queue3->location].isValid = 0;
            if (blockArray[queue2->location].isValid)
                addToQueue(2, queue2->location);
            
            queue2 = queue2->next_node;
        /* Do queue3 stuff */
        } else if (queue3) {
//            printf("Entering queue 3\n");
//            blockArray[queue0->location].isValid = 0;
//            blockArray[queue1->location].isValid = 0;
//            blockArray[queue2->location].isValid = 0;
            blockArray[queue3->location].isValid = 1;
            if (blockArray[queue3->location].isValid)
                addToQueue(3, queue3->location);
            
            queue3 = queue3->next_node;
        }
        
        /* Release finished processes and set the validity of that process in the bit vector back to 0. */
        int processID = waitpid(0, NULL, WNOHANG);
        if (processID > 0) {
            printf("Process %i finished execution.\n", processID);
            processCount--;
            for (index = 0; index < 18; index++) {
                if (blockArray[index].pid == processID) {
                    bitArray[index] = 0;
                    blockArray[index].isValid = 0;
                    break;
                }
            }
        }
//        sleep(1);
    }   /* End while loop */


    sleep(3);
    while ((wpid = wait(&status)) > 0);
    
    printf("Time is %i.%i\n", *seconds, *nano_seconds);
    printf("Finished waiting\n");

    detachEverything();
    return 0;
}

void addToQueue(int queueNum, int location) {
    procq_t *queue, *current;
    queue = (procq_t *)malloc(sizeof(procq_t));
    queue->location = location;
    //queue->process = process;
    queue->next_node = NULL;
 
    switch (queueNum) {
        case 0:
            if (!queue0)
                queue0 = queue;
            else {
                current = queue0;
                while (current->next_node)
                    current = current->next_node;
                current->next_node = queue;
            }
            break;
        case 1:
            if (!queue1)
                queue1 = queue;
            else {
                current = queue1;
                while (current->next_node)
                    current = current->next_node;
                current->next_node = queue;
            }
            break;
        case 2:
            if (!queue2)
                queue2 = queue;
            else {
                current = queue2;
                while (current->next_node)
                    current = current->next_node;
                current->next_node = queue;
            }
            break;
        case 3:
            if (!queue3)
                queue3 = queue;
            else {
                current = queue3;
                while (current->next_node)
                    current = current->next_node;
                current->next_node = queue;
            }
            break;
        
        default:
            printf("You are a genius for pulling off the impossible. I'm not even mad, I'm impressed.\n");
            break;
    }
}

void printQueue(procq_t *queue) {
    while (queue) {
        printf("queue->location: %i\n",queue->location);
        queue = queue->next_node;
    }
}

void newProcess(int processCount, int index) {
/*    PCB *block = malloc(sizeof(PCB));
    block->timeCreated = *seconds * 1000000000 + *nano_seconds;
    block->runTime = 0;
    block->timeElapsed = 0;
    block->burstTime = 0;
    block->timeInSystem = 0;
    block->isValid = 0;
    block->procID = processCount;
    block->pid = getpid();
*/
    blockArray[index].timeCreated = *seconds * 1000000000 + *nano_seconds;
    blockArray[index].runTime = 0;
    blockArray[index].timeElapsed = 0;
    blockArray[index].burstTime = 0;
    blockArray[index].timeInSystem = 0;
//    blockArray[index].didFinish = 0;
    blockArray[index].isValid = 0;
    blockArray[index].procID = processCount;
    blockArray[index].pid = getpid();
    
    /* 90% of the processes end up in queue 1 and about 10% in queue 0.
     Queues 2 and 3 are only available during runtime. */
    srand((unsigned)time(0));
    int queueChance = rand() % 100;
    if (queueChance >= 11) {
        //block->priority = 1;
        blockArray[index].priority = 1;
        addToQueue(1, index);
    }
    else {
        //block->priority = 0;
        blockArray[index].priority = 0;
        addToQueue(0, index);
    }
 
}

void removeProcess(int index) {
    blockArray[index].timeCreated = 0;
    blockArray[index].runTime = 0;
    blockArray[index].timeElapsed = 0;
    blockArray[index].burstTime = 0;
    blockArray[index].timeInSystem = 0;
    blockArray[index].priority = 4;
    blockArray[index].isValid = 0;
    blockArray[index].procID = 0;
    blockArray[index].pid = 0;
}

void printHelp() {
    printf("-h displays this help message\n");
    printf("-l filename sets the filename to be used for the log file\n");
//    printf("-m y sets the Integer argument 'y' to the variable 'm' for the amount the \"clock\"\n   will potentially increase by every time the loop iterates. (Increase time is a modulo of this number)\n");
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
