#include <stdio.h>

#include "callbacks.h"

void demoinfo_key(char *key) {
    printf("demoinfo key: %s\n", key);
}

void demoinfo_value(char *value) {
    printf("demoinfo value: %s\n", value);
}

void command(char *cmd, qbyte *targets, int target_count) {
    printf("command: %s\n", cmd);
}
