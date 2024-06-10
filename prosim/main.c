#include <stdio.h>
#include "context.h"


int main() {
    context *cur = context_load(stdin);

    if (!cur) {
        return -1;
    }

//    while (context_next_op(cur)) {
//        context_print(cur, stdout);
//    }
//    context_print(cur, stdout);
//
//    puts("");
//    context_stats(cur, stdout);
    return 0;
}