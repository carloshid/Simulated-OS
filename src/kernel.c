#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include "pcb.h"
#include "kernel.h"
#include "shell.h"
#include "shellmemory.h"
#include "interpreter.h"
#include "ready_queue.h"

bool multi_threading = false;
pthread_t worker1;
pthread_t worker2;
bool active = false;
bool debug = false;
bool in_background = false;
pthread_mutex_t queue_lock;

void lock_queue(){
    if(multi_threading) pthread_mutex_lock(&queue_lock);
}

void unlock_queue(){
    if(multi_threading) pthread_mutex_unlock(&queue_lock);
}

int process_initialize(char *filename){
    FILE* fp;
    int error_code = 0;
    int* pg0 = (int*)malloc(sizeof(int));
    int* pg1 = (int*)malloc(sizeof(int));
    int* size = (int*)malloc(sizeof(int));
    
    fp = fopen(filename, "rt");
    if(fp == NULL){
        error_code = 11; // 11 is the error code for file does not exist
        return error_code;
    }
    char* fileNameInBackingStore = calloc(1, 100);
    error_code = load_file(fp, size, pg0, pg1, filename, fileNameInBackingStore);

    if(error_code != 0){
        fclose(fp);
        return error_code;
    }
    
    PCB* newPCB = makePCB(*size, *pg0, *pg1, fileNameInBackingStore);
    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;

    lock_queue();
    ready_queue_add_to_tail(node);
    unlock_queue();
    fclose(fp);
    return error_code;
}

int shell_process_initialize(){
    //Note that "You can assume that the # option will only be used in batch mode."
    //So we know that the input is a file, we can directly load the file into ram
    int* pg0 = (int*)malloc(sizeof(int));
    int* pg1 = (int*)malloc(sizeof(int));
    int* size = (int*)malloc(sizeof(int));
    int error_code = 0;
    char* fileNameInBackingStore;
    error_code = load_file(stdin, size, pg0, pg1, "_SHELL", fileNameInBackingStore);
    if(error_code != 0){
        return error_code;
    }
    PCB* newPCB = makePCB(*size,*pg0,*pg1, fileNameInBackingStore);
    newPCB->priority = true;
    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;
    lock_queue();
    ready_queue_add_to_head(node);
    unlock_queue();
    freopen("/dev/tty", "r", stdin);
    return 0;
}

bool execute_process(QueueNode *node, int quanta){
    
    char *line = NULL;
    PCB *pcb = node->pcb;
    for(int i=0; i<quanta; i++){

        // Line number in the file
        int lineN = pcb->PC++;
        // Page number of the program
        int pgNumber = lineN / 3;
        // Offset within the page
        int pgOffset = lineN % 3;

        // Page number in memory
        int pg = pcb->pagetable[pgNumber];
        // Memory index
        int index;
        if (pg != -1) {
            index = getPageIndex(pg) + pgOffset;
            updatePageTime(pg);
        }
        else {
            // Page not in memory, load it
            int available = getFirstAvailablePage();
            if (available != -1) {
                // There is an empty page, use it
                loadProgramPage(pgNumber, available, pcb->fileNameInBackingStore);
                pcb->pagetable[pgNumber] = available;
                index = getPageIndex(available) + pgOffset;
            }
            else {
                // No empty page
                int emptied = evict_LRU();
                loadProgramPage(pgNumber, emptied, pcb->fileNameInBackingStore);
                pcb->pagetable[pgNumber] = emptied;
                index = getPageIndex(emptied) + pgOffset;

            }

            pcb->PC--;
            return false;
        }

        // Get line from memory
        line = mem_get_value_at_line(index);

        in_background = true;
        if(pcb->priority) {
            pcb->priority = false;
        }
        if(pcb->PC>(pcb->job_length_score)-1){
            parseInput(line);
            terminate_process(node);
            in_background = false;
            return true;
        }
        parseInput(line);
        in_background = false;
    }
    return false;
}

void *scheduler_FCFS(){
    QueueNode *cur;
    while(true){
        lock_queue();
        if(is_ready_empty()) {
            unlock_queue();
            if(active) continue;
            else break;   
        }
        cur = ready_queue_pop_head();
        unlock_queue();
        if (!execute_process(cur, MAX_INT)) {
            lock_queue();
            ready_queue_add_to_tail(cur);
            unlock_queue(); 
        };
    }
    if(multi_threading) pthread_exit(NULL);
    return 0;
}

void *scheduler_SJF(){
    QueueNode *cur;
    while(true){
        lock_queue();
        if(is_ready_empty()) {
            unlock_queue();
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_shortest_job();
        unlock_queue();
        execute_process(cur, MAX_INT);
    }
    if(multi_threading) pthread_exit(NULL);
    return 0;
}

void *scheduler_AGING_alternative(){
    QueueNode *cur;
    while(true){
        lock_queue();
        if(is_ready_empty()) {
            unlock_queue();
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_shortest_job();
        ready_queue_decrement_job_length_score();
        unlock_queue();
        if(!execute_process(cur, 1)) {
            lock_queue();
            ready_queue_add_to_head(cur);
            unlock_queue();
        }   
    }
    if(multi_threading) pthread_exit(NULL);
    return 0;
}

void *scheduler_AGING(){
    QueueNode *cur;
    int shortest;
    sort_ready_queue();
    while(true){
        lock_queue();
        if(is_ready_empty()) {
            unlock_queue();
            if(active) continue;
            else break;
        }
        cur = ready_queue_pop_head();
        shortest = ready_queue_get_shortest_job_score();
        if(shortest < cur->pcb->job_length_score){
            ready_queue_promote(shortest);
            ready_queue_add_to_tail(cur);
            cur = ready_queue_pop_head();
        }
        ready_queue_decrement_job_length_score();
        unlock_queue();
        if(!execute_process(cur, 1)) {
            lock_queue();
            ready_queue_add_to_head(cur);
            unlock_queue();
        }
    }
    if(multi_threading) pthread_exit(NULL);
    return 0;
}

void *scheduler_RR(void *arg){
    int quanta = ((int *) arg)[0];
    QueueNode *cur;
    while(true){
        lock_queue();
        if(is_ready_empty()){
            unlock_queue();
            if(active) continue;
            else break;
        }
        cur = ready_queue_get_head();
        unlock_queue();
        if(!execute_process(cur, quanta)) {
            lock_queue();
            cur = ready_queue_pop_head();
            ready_queue_add_to_tail(cur);
            unlock_queue();
        }
        else 
        {
           
            cur = ready_queue_pop_head();
            cur->pcb = NULL;
            cur->next = NULL;
        }
    }
    if(multi_threading) pthread_exit(NULL);
    return 0;
}

int threads_initialize(char* policy){
    active = true;
    multi_threading = true;
    int arg[1];
    pthread_mutex_init(&queue_lock, NULL);
    if(strcmp("FCFS",policy)==0){
        pthread_create(&worker1, NULL, scheduler_FCFS, NULL);
        pthread_create(&worker2, NULL, scheduler_FCFS, NULL);
    }else if(strcmp("SJF",policy)==0){
        pthread_create(&worker1, NULL, scheduler_SJF, NULL);
        pthread_create(&worker2, NULL, scheduler_SJF, NULL);
    }else if(strcmp("RR",policy)==0){
        arg[0] = 2;
        pthread_create(&worker1, NULL, scheduler_RR, (void *) arg);
        pthread_create(&worker2, NULL, scheduler_RR, (void *) arg);
    }else if(strcmp("AGING",policy)==0){
        pthread_create(&worker1, NULL, scheduler_AGING, (void *) arg);
        pthread_create(&worker2, NULL, scheduler_AGING, (void *) arg);
    }else if(strcmp("RR30", policy)==0){
        arg[0] = 30;
        pthread_create(&worker1, NULL, scheduler_RR, (void *) arg);
        pthread_create(&worker2, NULL, scheduler_RR, (void *) arg);
    }
}

void threads_terminate(){
    if(!active) return;
    bool empty = false;
    while(!empty){
        empty = is_ready_empty();
    }
    active = false;
    pthread_join(worker1, NULL);
    pthread_join(worker2, NULL);
}


int schedule_by_policy(char* policy, bool mt){
    if(strcmp(policy, "FCFS")!=0 && strcmp(policy, "SJF")!=0 && 
        strcmp(policy, "RR")!=0 && strcmp(policy, "AGING")!=0 && strcmp(policy, "RR30")!=0){
            return 15;
    }
    if(active) return 0;
    if(in_background) return 0;
    int arg[1];
    if(mt) return threads_initialize(policy);
    else{
        if(strcmp("FCFS",policy)==0){
            scheduler_FCFS();
        }else if(strcmp("SJF",policy)==0){
            scheduler_SJF();
        }else if(strcmp("RR",policy)==0){
            arg[0] = 2;
            scheduler_RR((void *) arg);
        }else if(strcmp("AGING",policy)==0){
            scheduler_AGING();
        }else if(strcmp("RR30", policy)==0){
            arg[0] = 30;
            scheduler_RR((void *) arg);
        }
        return 0;
    }

    mem_free_lines_between(0, FRAME_STORE_SIZE);
}

