#ifndef PLAYER_H
#define PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <graphx.h>
#include "cards.h"
#include "troops.h"
#include "spells.h"
#include "towers.h"
#include "game.h"
#include "structs.h"

// Game state might be needed to pass in the deck
player_t *init_player(bool opponent, card_t *deck);

#ifdef __cplusplus
}
#endif

#endif