#ifndef GAME_H
#define GAME_H

#ifdef __cplusplus
extern "C" {
#endif

#include "structs.h"
#include "../ui/menu.h"
#include "data.h"

game_t *init_game(data_t *data);
void run_game(game_t *game);
void start_game(void *args);
void free_data(data_t *data);

#ifdef __cplusplus
}
#endif

#endif