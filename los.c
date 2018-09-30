#include "los.h"
#include <stdio.h>

/*
void draw_line(level *lvl, int cur_x, int cur_y, int target_x, int target_y) {
    slope = slope(cur_x, cur_y, target_x, target_y)
    y_step = slope / (target_x - cur_x)
    // one step?
    acc_error = 0
    until next_square = end
        next_square(&cur_x, &cur_y, y_step, &acc_error)
        draw(lvl, cur_x, cur_y)
*/

void next_square(int *x, int *y, int x_step, float y_step, int *acc_err) {
    // Change a position to the next position along an angle (sort of)
    // This requires y_step and acc_err to be passed in so this really can't
    // exist on its own. That feels weird.

    // sanity check
    fprintf(stderr, "%s((%d, %d), %d, %.2f, %d)\n", "next_square", *x, *y, x_step, y_step, *acc_err);

    if (y_step < 0) {
        fprintf(stderr, "%s: y_step is negative\n", "next_square");
        return;
    } else if (y_step == 0) {
        fprintf(stderr, "%s: y_step is zero\n", "next_square");
        return;
    } else if (y_step == INFINITY) {
        fprintf(stderr, "%s: y_step is infinity\n", "next_square");
        return;
    }

    int ideal_y, adjust;

    // *x++ <-- Nooooooooooooooo!
    *x += x_step;
    ideal_y = *y + y_step;

    *y = round(ideal_y);
    // y = round(ideal_y = *y + y_step);  :.(

    *acc_err += *y - ideal_y;

    if (abs(*acc_err > 0.5)) {
        adjust = *acc_err < 0 ? 1 : -1;

        *y += adjust;
        *acc_err += adjust;
    }
}
