#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "mob/mob.h"
#include "level/level.h"

#include "curses_renderer.h"
#include "game.h"
#include "log.h"
#include "input.h"
#include "simulation/simulation.h"
#include "los/los.h"
#include "colors/colors.h"


void step_chemistry(chemical_system *sys, constituents *chem, constituents *context) {
    for (int i = 0; i < 3; i++) {
        bool is_stable = chem->stable;
        if (context != NULL) is_stable = is_stable && context->stable;
        if (!is_stable) {
            react(sys, chem, context);
        }
    }
}

void step_inventory_chemistry(chemical_system *sys, inventory_item *inv, constituents *context) {
    while (inv != NULL) {
        step_chemistry(sys, inv->item->chemistry, context);
        inv = inv->next;
    }
}

void step_item(level *lvl, item *itm, constituents *chem_ctx) {
    step_chemistry(lvl->chem_sys, itm->chemistry, chem_ctx);
    step_inventory_chemistry(lvl->chem_sys, itm->contents, itm->chemistry);
    bool burning = itm->chemistry->elements[fire] > 0;
    if (chem_ctx != NULL) burning = burning || chem_ctx->elements[fire] > 0;
    if (burning && itm->health > 0) item_deal_damage(lvl, itm, 1);
}

void step_mobile(level *lvl, mobile *mob) {
    constituents *chemistry = ((item*)mob)->chemistry;
    if (lvl->chemistry[mob->x][mob->y]->elements[air] > 5) {
        lvl->chemistry[mob->x][mob->y]->elements[air] -= 5;
    } else {
        item_deal_damage(lvl, ((item*)mob), 1);
    }
    if (chemistry->elements[life] > 0) {
        chemistry->elements[life] -= 10;
        ((item*)mob)->health += 1;
    }
    if (chemistry->elements[venom] > 0) {
        chemistry->elements[venom] -= 10;
        item_deal_damage(lvl, ((item*)mob), 1);
    }
    step_item(lvl, (item*)mob, lvl->chemistry[mob->x][mob->y]);
    if (((item*)mob)->health <= 0) {
        mob->active = false;
    }
}

void level_step_chemistry(level* lvl) {
    for (int x = 0; x < lvl->width; x++) {
        for (int y = 0; y < lvl->height; y++) {
            step_chemistry(lvl->chem_sys, lvl->chemistry[x][y], NULL);
            inventory_item *inv = lvl->items[x][y];
            while (inv != NULL) {
                step_item(lvl, inv->item, lvl->chemistry[x][y]);
                if (inv->item->health <= 0) {
                    inv->item->name = "Ashy Remnants";
                    //TODO Hard-coded icon
                    inv->item->display = '~';
                }
                inv = inv->next;
            }
            if (lvl->tiles[x][y] != WALL && lvl->tiles[x][y] != CLOSED_DOOR && lvl->chemistry[x][y]->elements[air] < 20) lvl->chemistry[x][y]->elements[air] += 3;
        }
    }
    for (int element = 0; element < ELEMENT_COUNT; element++) {
        if (lvl->chem_sys->volitile[element]) {
            int **added_element = malloc(lvl->width * sizeof(int*));
            added_element[0] = malloc(lvl->height * lvl->width * sizeof(int));
            int **removed_element = malloc(lvl->width * sizeof(int*));
            removed_element[0] = malloc(lvl->height * lvl->width * sizeof(int));
            for(int i = 1; i < lvl->width; i++) {
                added_element[i] = added_element[0] + i * lvl->height;
                removed_element[i] = removed_element[0] + i * lvl->height;
            }
            for (int x = 0; x < lvl->width; x++) {
                for (int y = 0; y < lvl->height; y++) {
                    added_element[x][y] = 0;
                    removed_element[x][y] = 0;
                }
            }

            for (int x = 0; x < lvl->width; x++) {
                for (int y = 0; y < lvl->height; y++) {
                    int rx = rand();
                    int ry = rand();
                    for (int dx = 0; dx < 2; dx++) {
                        int xx = x + (((dx+rx)%3)-1);
                        for (int dy = 0; dy < 2; dy++) {
                            int yy = y + (((dy+ry)%3)-1);
                            if (xx >= 0 && xx < lvl->width && yy >= 0 && yy < lvl->height) {
                                if (lvl->tiles[xx][yy] != WALL && lvl->tiles[xx][yy] != CLOSED_DOOR && lvl->chemistry[x][y]->elements[element] - removed_element[x][y] > lvl->chemistry[xx][yy]->elements[element] + added_element[xx][yy]) {
                                    removed_element[x][y] += 1;
                                    added_element[xx][yy] += 1;
                                }
                            }
                        }
                    }
                }
            }
            for (int x = 0; x < lvl->width; x++) {
                for (int y = 0; y < lvl->height; y++) {
                    if (added_element[x][y] > 0 || removed_element[x][y] > 0) {
                        lvl->chemistry[x][y]->elements[element] += added_element[x][y] - removed_element[x][y];
                        lvl->chemistry[x][y]->stable = false;
                    }
                }
            }
            free((void*)added_element[0]);
            free((void*)added_element);
            free((void*)removed_element[0]);
            free((void*)removed_element);
        }
    }


}

int main() {
        int ch;
        int turn = 0;
        level *lvl;
        const char* do_log = getenv("SHITTY_LOG");

        if (do_log != NULL) logging_active = true;

        srand(time(NULL));

        init_rendering_system();

        lvl = make_level();
        draw_level(lvl);

        // Main Loop
        while (lvl->active) {
            logger("=== Turn %3d ============================================\n", turn++);
            turn++;
            sync_simulation(lvl->sim, turn*TICKS_PER_TURN);

            for (int i=0; i < lvl->mob_count; i++) {
                if (lvl->mobs[i]->active) {
                    step_mobile(lvl, lvl->mobs[i]);
                }
            }

            // Update chemistry model
            level_step_chemistry(lvl);

            // Prep death message if player is dead
            if (!lvl->player->active) {
                print_message("You die");
            }

            // Update the screen
            draw_level(lvl);

            // If the player is dead, wait for input
            if (!lvl->player->active) {
                get_input(lvl);
                break;
            }

            // Clear message
            print_message("");

            get_input(lvl);
        }

        destroy_level(lvl);

        cleanup_rendering_system();
        return 0;
}
