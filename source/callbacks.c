#include <stdio.h>

#include "callbacks.h"

void command(char *cmd, qbyte *targets, int target_count) {
    printf("%s\n", cmd);
}
