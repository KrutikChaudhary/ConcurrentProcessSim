//
// Created by Alex Brodsky on 2023-04-02.
//

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "context.h"

static const char *OPS [] = {"HALT", "DOOP", "LOOP", "END", "BLOCK", NULL};

#define PUSH(s,v) (*(s++) = v)
#define POP(s) (*(--s))
#define PEEK(s) (*(s - 1))


typedef struct {
    context *priorityQueue[100];
    int size;
} PriorityQueue;

PriorityQueue* rq;
PriorityQueue* bq;

void swap(context **a, context **b) {
    context *temp = *a;
    *a = *b;
    *b = temp;
}

void bubbleUp(PriorityQueue *queue, int index) {
    while (index > 0 && queue->priorityQueue[index]->priority < queue->priorityQueue[(index - 1) / 2]->priority) {
        swap(&queue->priorityQueue[index], &queue->priorityQueue[(index - 1) / 2]);
        index = (index - 1) / 2;
    }
}

void enqueue(PriorityQueue *queue, context *ctx) {
    if (queue->size >= 100) {
        printf("Queue is full\n");
        return;
    }
    queue->priorityQueue[queue->size] = ctx;
    bubbleUp(queue, queue->size);
    queue->size++;
}

void bubbleDown(PriorityQueue *queue, int index) {
    int smallest = index;
    int leftChildIdx = 2 * index + 1;
    int rightChildIdx = 2 * index + 2;

    if (leftChildIdx < queue->size && queue->priorityQueue[leftChildIdx]->priority < queue->priorityQueue[smallest]->priority) {
        smallest = leftChildIdx;
    }
    if (rightChildIdx < queue->size && queue->priorityQueue[rightChildIdx]->priority < queue->priorityQueue[smallest]->priority) {
        smallest = rightChildIdx;
    }

    if (smallest != index) {
        swap(&queue->priorityQueue[index], &queue->priorityQueue[smallest]);
        bubbleDown(queue, smallest);
    }
}

context *dequeue(PriorityQueue *queue) {
    if (queue->size == 0) {
        return NULL;
    }
    context *top = queue->priorityQueue[0];
    swap(&queue->priorityQueue[0], &queue->priorityQueue[queue->size - 1]);
    queue->size--;
    bubbleDown(queue, 0);
    return top;
}

int time = 0;
context *contexts;
int numProcs;
int quantum;
extern context *context_load(FILE *fin) {


    fscanf(fin, "%d %d", &numProcs, &quantum);
    contexts = calloc(numProcs, sizeof(context));
    for(int i=0; i<numProcs; i++){
        int size;
        fscanf(fin, "%10s %d %d", contexts[i].name, &size, &contexts[i].priority);

        contexts[i].stack = malloc(2 * sizeof(int) * size);
        assert(contexts[i].stack);

        contexts[i].code = malloc(size * sizeof(opcode));
        assert(contexts[i].code);
        contexts[i].ip = -1;
        contexts[i].idx = i + 1;
        strcpy(contexts[i].state, "new");

        for (int j = 0; j < size; j++) {
            char op[10];
            fscanf(fin, "%9s", op);

            contexts[i].code[j].op = -1;
            for (int k = 0; OPS[k]; k++) {
                if (!strcmp(op, OPS[k])) {
                    contexts[i].code[j].op = k;
                    if (k == OP_LOOP || k == OP_DOOP || k == OP_BLOCK) {
                        fscanf(fin, "%d", &contexts[i].code[j].arg);
                    }
                    break;
                }
            }


            if (contexts[i].code[j].op == -1) {
                printf("error, operation %d unknown: %s\n", j + 1, op);
                return NULL;
            }
        }
    }


    rq = (PriorityQueue*)malloc(sizeof(PriorityQueue));
    bq = (PriorityQueue*)malloc(sizeof(PriorityQueue));

    rq->size = 0;
    bq->size = 0;

    //add to respective queue
    for(int i = 0; i<numProcs; i++){
        //printf("%s\n",contexts[i].name);
        printf("%5.5d: process %d %s\n",time,contexts[i].idx, contexts[i].state);

        //find q to place
        for(int j = -1; 1; j++){
            if(contexts[i].code[j].op==OP_DOOP){
                strcpy(contexts[i].state, "ready");
                enqueue(rq,&contexts[i]);
                break;
            } else if (contexts[i].code[j].op==OP_BLOCK){
                strcpy(contexts[i].state, "blocked");
                enqueue(bq,&contexts[i]);
                break;
            }
        }

        printf("%5.5d: process %d %s\n",time,contexts[i].idx, contexts[i].state);
    }

    int quanT = 0;
    context *current;
    while(!allFinished(contexts)){

        if(quanT==0 && !strcmp(rq->priorityQueue[0]->state,"ready")){
            strcpy(rq->priorityQueue[0]->state, "running");
            printf("%5.5d: process %d %s\n",time,rq->priorityQueue[0]->idx, rq->priorityQueue[0]->state);
        }
        for(int i = 0; i<numProcs; i++){
            if(!strcmp(contexts[i].state,"blocked")){
                //printf("%d\n",rq->size);
                context_next_op(&contexts[i]);
            }
        }
        if(!strcmp(rq->priorityQueue[0]->state, "ready")){
            strcpy(rq->priorityQueue[0]->state, "running");
            printf("%5.5d: process %d %s\n",time,rq->priorityQueue[0]->idx, rq->priorityQueue[0]->state);
        }
        else if(rq->size>0){
            context_next_op(rq->priorityQueue[0]);
        }

        time++;
        quanT++;

        //printf("%dq\n",time);

        if(quanT>=quantum ){//reset quantum

            quanT=0;
            if(rq->priorityQueue[0]->code[rq->priorityQueue[0]->ip].op==OP_DOOP){
                //printf("%d %d arg\n",rq->priorityQueue[0]->code[rq->priorityQueue[0]->ip].arg,rq->priorityQueue[0]->code[rq->priorityQueue[0]->ip].op );
                if(rq->priorityQueue[0]->code[rq->priorityQueue[0]->ip].arg > 1){
                    strcpy(rq->priorityQueue[0]->state, "ready");
                    printf("%5.5d: process %d %s\n",time,rq->priorityQueue[0]->idx, rq->priorityQueue[0]->state);
                }
            }
        }


    }

    for(int i=0; i<numProcs; i++){
        printf("Process %d: Run time %d, Block time %d, Wait time %d\n",contexts[i].idx,contexts[i].run_time,contexts[i].block_time,contexts[i].wait_time );
    }




    return contexts;
}

extern int allFinished(context *cur){
    for(int i = 0; i<numProcs; i++){
        if(strcmp(cur[i].state,"finished")){
            return 0;
        }
    }

    return 1;
}

extern int context_next_op(context *cur) {
    int count;

    if (cur->ip >= 0) {
        //cur->clock += cur->code[cur->ip].arg;
    } else {
        cur->ip++;
    }

    for (;;) {

        switch (cur->code[cur->ip].op) {
            case OP_LOOP:
                PUSH(cur->stack, cur->ip);
                PUSH(cur->stack, cur->code[cur->ip].arg);
                cur->ip++;
                break;
            case OP_DOOP:
                if(!strcmp(cur->state,"blocked")){
                    strcpy(cur->state, "ready");
                    printf("%5.5d: process %d %s\n",time,cur->idx, cur->state);
                    enqueue(rq,cur);

                    //time++;
                    //printf("%d  oyy\n",rq->size);
                }
                else if(strcmp(cur->state,"running")){
                    strcpy(cur->state, "running");
                    printf("%5.5d: process %d %s\n",time,cur->idx, cur->state);

                }
                cur->run_time++;

                cur->code[cur->ip].arg--;
                if(cur->code[cur->ip].arg==0){
                    cur->ip++;
                }
                return 1;
            case OP_BLOCK:
                cur->block_time++;
                if(strcmp(cur->state,"blocked")){
                    strcpy(cur->state, "blocked");
                    printf("%5.5d: process %d %s\n",time,cur->idx, cur->state);
                    dequeue(rq);
                    //return 1;
                }
                cur->code[cur->ip].arg--;
                if(cur->code[cur->ip].arg==0){
                    cur->ip++;
                }
                return 1;

//                if(cur->code->arg==0) {
//                    cur->ip++;
//                    for(int i = cur->ip; ; i++){
//                        if(cur->code[i].op==OP_DOOP){
//
//                        } else if(cur->code[i].op==OP_BLOCK) {
//
//                        } else if(cur->code[i].op==OP_HALT){
//
//                        }
//                    }
//                    strcpy(cur->state, "ready");
//                    printf("%5.5d: process %d %s\n",time,cur->idx, cur->state);
//                    enqueue(rq,cur);
//                }

//                cur->block_count++;
//                cur->block_time += cur->code[cur->ip].arg;
                return 1;
            case OP_END:
                count = POP(cur->stack);
                count--;
                if (count == 0) {
                    POP(cur->stack);
                    cur->ip++;
                } else {
                    cur->ip = PEEK(cur->stack);
                    PUSH(cur->stack, count);
                }
                break;
            case OP_HALT:
                strcpy(cur->state, "finished");
                printf("%5.5d: process %d %s\n",time,cur->idx, cur->state);
                dequeue(rq);
                return 0;
            default:
                printf("error, unknown opcode %d at ip %d\n", cur->code[cur->ip].op, cur->ip);
                return -1;
        }
    }
}

extern int context_print(context *cur, FILE *fout) {
    switch (cur->code[cur->ip].op) {
        case OP_DOOP:
        case OP_BLOCK:
        case OP_HALT:
//            fprintf(fout, "%5.5d: %s\n", cur->clock, OPS[cur->code[cur->ip].op]);
            break;
        case OP_LOOP:
        case OP_END:
            break;
        default:
            printf("error, unknown opcode %d at ip %d\n", cur->code[cur->ip].op, cur->ip);
            return -1;
    }
    return 0;
}

extern void context_stats(context *cur, FILE *fout) {
//    fprintf(fout, "DOOP count : %d\n", cur->doop_count);
//    fprintf(fout, "DOOP time  : %d\n", cur->doop_time);
//    fprintf(fout, "BLOCK count: %d\n", cur->block_count);
    fprintf(fout, "BLOCK time : %d\n", cur->block_time);
}

