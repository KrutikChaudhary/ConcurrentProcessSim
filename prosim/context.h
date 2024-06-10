//
// Created by Alex Brodsky on 2023-04-02.
//

#ifndef ASSIGNMENT_1_CONTEXT_H
#define ASSIGNMENT_1_CONTEXT_H

#include <stdio.h>

enum {
    OP_HALT, OP_DOOP, OP_LOOP, OP_END, OP_BLOCK, OP_LAST
};

typedef struct opcode {
    int op;
    int arg;
} opcode;

typedef struct context {
    opcode *code;
    int *stack;
    char name[11];
    int priority;
    char state[15];
    int ip;
    int clock;
    int run_time;
    int wait_time;
    int block_time;
    int idx;
} context;

extern int context_next_op(context *cur);
extern int context_print(context *cur, FILE *fout);
extern context *context_load(FILE *fin);
extern void context_stats(context *cur, FILE *fout);


#endif //ASSIGNMENT_1_CONTEXT_H
