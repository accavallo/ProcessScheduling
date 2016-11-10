//  oss.c
//  Proj4
//
//  Created by Tony on 10/25/16.
//  Copyright © 2016 Anthony Cavallo. All rights reserved.
//

#include "Proj4.h"

static procq_t *queue0 = NULL, *queue1 = NULL, *queue2 = NULL, *queue3 = NULL;
static long long unsigned processor_idle_time = 0, average_wait_time = 0, average_turnaround = 0;
static int totalProcesses = 0;

int main(int argc, const char * argv[]) {
    signal(SIGSEGV, faultHandler);
    signal(SIGINT, interruptHandler);
    
    int option, index;
/*    char *fileName = "logfile.txt";
    
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
 */
 
    /* Set up shared memory */
    /******** NOTE: *********
     seconds_memory ONLY NEEDS TO BE sizeof(int) * 3 (seconds, nano_seconds, and quantum)! block_memory needs only be sizeof(PCB) * 18
     ************************/
    if ((seconds_memory = shmget(TIMEKEY, sizeof(int) * 3, IPC_CREAT | 0777)) == -1) {
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
    quantum = seconds + 2;
    
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
    
    char procID[5];
    int bitArray[18] = {0}, processor_idle = 1, processCount = 0, status;
    long long unsigned creationTime = 0, currentTime = 0, creationTimeSet = 0, previousTime = 0;
    pid_t pid, wpid;
    
    srand((unsigned)time(0));
    alarm(ALARM_DURATION);
    signal(SIGALRM, alarmHandler);
    
    while (*seconds < 10) {
        if (creationTimeSet == 0) {
            int newTime = rand() % 2000000000;
            sleep(1);
            creationTime = *seconds * 1000000000 + *nano_seconds + newTime;
            creationTimeSet = 1;
            totalProcesses++;
            printf("Creation time of process %i is set for %llu, %i nanoseconds ahead.\n", totalProcesses, creationTime, newTime);
        }
        currentTime = *seconds * 1000000000 + *nano_seconds;
        
//        printf("currentTime: %llu\n", currentTime);
        
        /* No more than 18 processes at a time, and the current time needs to be after the creation time that was set "randomly". */
        if (processCount < MAX_PROCESSES && creationTime <= currentTime) {
            for (index = 0; index < 18; index++)
                if(bitArray[index] == 0) {
                    bitArray[index] = 1;
                    break;
                }
            
            pid = fork();
            sprintf(procID, "%i", processCount);
            processCount++;
            creationTimeSet = 0;
            newProcess(totalProcesses, index);        //Not necessary, but it cleans up my main.
            if (pid == 0) {
                execl("./user", procID, (char *)NULL);
                removeProcess(index);          //If the execl fails, this will essentially clear that process control block.
                exit(EXIT_FAILURE);
            }
        }
        /* Every node, after it's time slice, will move to the next node in the queue. */
        /* QUEUE 0 WILL STOP EVERY OTHER QUEUE FROM RUNNING! QUEUES 1, 2, AND 3 WILL BE A MULTILEVEL FEEDBACK QUEUE */
        
        /* Do queue0 stuff */
        if (queue0) {
            previousTime = currentTime;
            currentTime = advanceTime(0);
            *quantum = (unsigned int)(currentTime - previousTime);
            queue0 = advanceProcess(queue0);
            processor_idle = 0;
            
        } else {
            /* Advance queue1 processes at a certain rate. Queue2 will be double that time, and queue3 will be double that of queue2. */
            /* Do queue1 stuff */
            if (queue1) {
                previousTime = currentTime;
                currentTime = advanceTime(1);
                *quantum = (unsigned int)(currentTime - previousTime);
                queue1 = advanceProcess(queue1);
                processor_idle = 0;
            }
            /* Do queue2 stuff */
            if (queue2) {
                previousTime = currentTime;
                currentTime = advanceTime(1);
                *quantum = (unsigned int)(currentTime - previousTime);
                queue2 = advanceProcess(queue2);
                processor_idle = 0;
            }
            /* Do queue3 stuff */
            if (queue3) {
                previousTime = currentTime;
                currentTime = advanceTime(3);
                *quantum = (unsigned int)(currentTime - previousTime);
                queue3 = advanceProcess(queue3);
            }
        }
        /* Release finished processes and set the validity of that process in the bit vector back to 0. */
        int processID = waitpid(0, NULL, WNOHANG);
        if (processID > 0) {
            processCount--;
            for (index = 0; index < 18; index++) {
                if (blockArray[index].pid == processID) {
                    printf("Process %i finished execution.\n", blockArray[index].procID);
                    bitArray[index] = 0;
                    blockArray[index].isValid = 0;
                    average_wait_time += blockArray[index].waitTime;
                    average_turnaround += ((*seconds * 1000000000 + *nano_seconds) - blockArray[index].timeCreated);
                    removeProcess(index);
                    break;
                }
            }
        }
        previousTime = currentTime;
        
        /* If all 4 queues are empty then the processor is idle. In which case the processor needs to advance it's time. */
        if (!queue3 && !queue2 && !queue1 && !queue0)
            processor_idle_time = advanceTime(5);
        
    }   /* End while loop */


    /* This code should never actually be executed, but in the off chance that it is... */
    sleep(3);
    while ((wpid = wait(&status)) > 0);
    printf("Finished waiting\n");

    detachEverything();
    return 0;
}

void addToQueue(int queueNum, int location) {
    procq_t *queue, *current;
    queue = (procq_t *)malloc(sizeof(procq_t));
    queue->location = location;
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
//            printf("Queue 0\n");
//            printQueue(queue0);
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
//            printf("Queue 1\n");
//            printQueue(queue1);
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
//            printf("Queue 2\n");
//            printQueue(queue2);
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
//            printf("Queue 3\n");
//            printQueue(queue3);
            break;
        
        default:
            printf("You are a genius for pulling off the impossible. I'm not even mad, I'm impressed.\n");
            break;
    }
}

procq_t * advanceProcess(procq_t *queue) {
    blockArray[queue->location].isValid = 1;
//    blockArray[queue->location].timeElapsed += timeElapsed;
    if (blockArray[queue->location].isValid && !blockArray[queue->location].didFinish)
        addToQueue(blockArray[queue->location].priority, queue->location);
    
    blockArray[queue->location].isValid = 0;
    queue = queue->next_node;
    *nano_seconds -= *quantum;
    return queue;
}

long long unsigned advanceTime(int queue) {
    /* Instead of trying to get each queue to *hopefully* use up their entire time slice, this will ensure that each queue will use exactly a certain amount of time. Queue2 will be twice that of queue1 and queue3 will be twice that of queue2. Queue0 has no relation with the other time slices. */
    switch (queue) {
        case 0:
            *nano_seconds += QUEUE0_QUANTUM;
            break;
        case 1:
            *nano_seconds += TIME_SLICE;
            break;
        case 2:
            *nano_seconds += TIME_SLICE * 2;
            break;
        case 3:
            *nano_seconds += TIME_SLICE * 4;
            break;
        default:
            *nano_seconds += rand() % TIME_INCREMENT;
            break;
    }
    
    if (*nano_seconds >= 1000000000) {
        (*seconds)++;
        *nano_seconds %= 1000000000;
    }
    return *seconds * 1000000000 + *nano_seconds;
}
void printQueue(procq_t *queue) {
    while (queue) {
        printf("queue->location: %i\n",queue->location);
        queue = queue->next_node;
    }
}

void newProcess(int processCount, int index) {
    srand((unsigned)time(0));
    
    blockArray[index].timeCreated = *seconds * 1000000000 + *nano_seconds;
    blockArray[index].waitTime = 0;
    blockArray[index].timeElapsed = 0;
    blockArray[index].burstTime = 0;
    blockArray[index].timeInSystem = 0;
    blockArray[index].didFinish = 0;
    blockArray[index].isValid = 0;
    blockArray[index].procID = processCount;
    blockArray[index].pid = getpid();
    
    /* 90% of the processes end up in queue 1 and about 10% in queue 0.
     Queues 2 and 3 are only available during runtime. */
    int queueChance = rand() % 100;
    if (queueChance > PRIORITY_CHANCE) {
        blockArray[index].priority = 1;
        addToQueue(1, index);
    }
    else {
        blockArray[index].priority = 0;
        addToQueue(0, index);
    }
}

void removeProcess(int index) {
    blockArray[index].timeCreated = 0;
    blockArray[index].waitTime = 0;
    blockArray[index].timeElapsed = 0;
    blockArray[index].burstTime = 0;
    blockArray[index].didFinish = 1;
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
    printf("\nDo you want to go to war Balake? I'm for real.\n");
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
    /* Regardless of whether this thing will finish normally, or not (it never will), we need to print out the processor idle time and other things like that. */
    printf("Time is %i.%i\n", *seconds, *nano_seconds);
    printf("Processor idle time: %llu nanoseconds.\n", processor_idle_time);
    printf("Average wait time: %.02f nanoseconds.\n", (double)(average_wait_time / totalProcesses));
    printf("Average turnaround time: %.02f nanoseconds.\n", (double)(average_turnaround / totalProcesses));
    /* Detach from and remove shared memory */
    shmdt(seconds);
    shmdt(nano_seconds);
    shmdt(quantum);
    shmctl(seconds_memory, IPC_RMID, NULL);     //Remove shared memory
    
    shmdt(blockArray);
    shmctl(block_memory, IPC_RMID, NULL);       //Remove the block memory
}
