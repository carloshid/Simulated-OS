#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pcb.h"

int pid_counter = 1;

int generatePID(){
    return pid_counter++;
}

//In this implementation, Pid is the same as file ID 
PCB* makePCB(int size, int pg0, int pg1, char *filename){
    PCB * newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->PC = 0;
    newPCB->job_length_score = size;
    newPCB->priority = false;
    int npages = ((size-1)/ 3 + 1);
    int *pagetable = malloc(npages * sizeof(int));
    for (int i = 0; i < npages; i++) {
        pagetable[i] = -1;
    }
    pagetable[0] = pg0;
    pagetable[1] = pg1;
    newPCB->npages = npages;
    newPCB->pagetable = pagetable;
    newPCB->fileNameInBackingStore = filename;
    return newPCB;
}