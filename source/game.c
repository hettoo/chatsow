#include "game.h"

entity_state_t baselines[MAX_EDICTS];

void game_init() {
    memset(baselines, 0, sizeof(baselines));
}
