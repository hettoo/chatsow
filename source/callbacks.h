#ifndef WDP_CALLBACKS_H
#define WDP_CALLBACKS_H

#include "import.h"

void demoinfo_key(char *key);
void demoinfo_value(char *value);
void command(char *cmd, qbyte *targets, int target_count);

#endif
