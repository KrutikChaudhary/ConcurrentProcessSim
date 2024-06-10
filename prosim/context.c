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
extern context *context_load(FILE *fin) {
    int numProcs;
    int quantum;

    fscanf(fin, "%d %d", &numProcs, &quantum);
    context *contexts = calloc(numProcs, sizeof(context));
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
    context *current = rq->priorityQueue[0];
//    while(rq->size!=0 && bq->size!=0){
//        printf("%5.5d: process %d %s\n",time,current->idx, current->state);
//        for(int i = 0; i<bq->size; i++){
//            context *
//        }
//
//    }
    return contexts;
}

extern int context_next_op(context *cur) {
    int count;

    if (cur->ip >= 0) {
        cur->clock += cur->code[cur->ip].arg;
    }

    for (;;) {
        cur->ip++;
        switch (cur->code[cur->ip].op) {
            case OP_LOOP:
                PUSH(cur->stack, cur->ip);
                PUSH(cur->stack, cur->code[cur->ip].arg);
                break;
            case OP_DOOP:
//                cur->doop_count++;
//                cur->doop_time += cur->code[cur->ip].arg;
                return 1;
            case OP_BLOCK:
//                cur->block_count++;
//                cur->block_time += cur->code[cur->ip].arg;
                return 1;
            case OP_END:
                count = POP(cur->stack);
                count--;
                if (count == 0) {
                    POP(cur->stack);
                } else {
                    cur->ip = PEEK(cur->stack);
                    PUSH(cur->stack, count);
                }
                break;
            case OP_HALT:
                printf("%5.5d: process %d %s\n",time,cur->idx, cur->state);
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
            fprintf(fout, "%5.5d: %s\n", cur->clock, OPS[cur->code[cur->ip].op]);
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

