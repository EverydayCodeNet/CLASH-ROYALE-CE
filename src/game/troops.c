#include "troops.h"
#include "structs.h"

void update_miner_movement(troop_t *miner, position_t target) {
    if (miner == NULL) return;
    
    // Move diagonally to x position first, then straight on y
    if (miner->position.x != target.x) {
        int dx = (target.x > miner->position.x) ? 1 : -1;
        miner->position.x += dx;
        
        // Also move y diagonally until x is reached
        if (miner->position.y != target.y) {
            int dy = (target.y > miner->position.y) ? 1 : -1;
            miner->position.y += dy;
        }
    } else if (miner->position.y != target.y) {
        // Move straight on y axis
        int dy = (target.y > miner->position.y) ? 1 : -1;
        miner->position.y += dy;
    }
}

bool is_miner_underground(troop_t *troop) {
    // Miner is underground during initial spawn animation
    // With sprite sheet system, we can't check specific sprite pointer
    // Just use movement_ticks as the indicator
    return (troop->movement_ticks < 60);
}
