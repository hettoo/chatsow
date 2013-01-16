#include <stdio.h>

#include "callbacks.h"

void demoinfo_key(char *key) {
    printf("demoinfo key %s\n", key);
}

void demoinfo_value(char *value) {
    printf("demoinfo value %s\n", value);
}

void command(char *cmd, qbyte *targets, int target_count) {
    printf("cmd %d", target_count);
    int i;
    for (i = 0; i < target_count; i++)
        printf(" %d", targets[i]);
    printf(" %s\n", cmd);
}
