// Based on:
//
//  https://www.cs.unm.edu/~angel/BOOK/INTERACTIVE_COMPUTER_GRAPHICS/FOURTH_EDITION/PROGRAMS/bresenham.c
//
// Compile via:
//
//  cc braisingham.c -lGL -lGLU -lglut -o show && ./show`
//

#include <GL/glut.h>
#include <stdio.h>
#include <stdbool.h>

#define PIXELS_PER_SQUARE 10
#define WINDOW_WIDTH_IN_SQUARES 50
#define WINDOW_HEIGHT_IN_SQUARES 50

#define WINDOW_WIDTH (WINDOW_WIDTH_IN_SQUARES * PIXELS_PER_SQUARE)
#define WINDOW_HEIGHT (WINDOW_HEIGHT_IN_SQUARES * PIXELS_PER_SQUARE)

void draw_pixel(int x, int y, float red, float green, float blue) {
    //fprintf(stderr, "Drawing (%3d,%3d)\n", x, y);
    glBegin(GL_POINTS);
        glColor3f(red, green, blue);
        glVertex2i(x, y);
    glEnd();
}

bool checker(int x, int y) {
    if (x > y) {
        return false;
    } else {
        return true;
    }
}

void doer(int x, int y) {
    glPointSize((float)PIXELS_PER_SQUARE / 3);
    draw_pixel(x, y, 0.0, 0.0, 0.0);
    glPointSize((float)PIXELS_PER_SQUARE);
}

bool braise(int a_x, int a_y, int b_x, int b_y) {
    // initialize starting (x,y)
    int x = a_x * PIXELS_PER_SQUARE;
    int y = a_y * PIXELS_PER_SQUARE;

    // calculate x- and y-distances
    int dx = abs(b_x - a_x);
    int dy = abs(b_y - a_y);

    // set signs on increments
    int x_increment = PIXELS_PER_SQUARE * ((b_x >= a_x) ? 1 : -1);
    int y_increment = PIXELS_PER_SQUARE * ((b_y >= a_y) ? 1 : -1);

    // pointers to things what get changed
    int *rise, *run;
    int *stepper, *step;
    int *bumper, *bump;

    // starting line color
    float red = 0.0;
    float green = 1.0;
    float blue = 0.0;

    fprintf(stderr, "Initialized\na=(%d,%d) b=(%d,%d) xy=(%d,%d) dxy=(%d,%d) inc_xy(%d,%d)\n",a_x,a_y,b_x,b_y,x,y,dx,dy,x_increment,y_increment);

    // set up which values will be modified based on
    // the slope of the line
    if (dx > dy) {
        // The slope is shallow
        rise = &dy;
        run = &dx;

        stepper = &x;
        step = &x_increment;

        bumper = &y;
        bump = &y_increment;
    } else {
        // The slope is steep (or 45 degrees)
        rise = &dx;
        run = &dy;

        stepper = &y;
        step = &y_increment;

        bumper = &x;
        bump = &x_increment;
    }

    // multiply everything by 2 to get rid of the 0.5 and
    // leave only integer math (and shifts)
    int error = (2 * *rise) - *run;

    // error adjustements
    int increment = 2 * *rise;
    int drain = 2 * *run;

    // size of color adjustment per step
    float color_step = 1.0 / *run;

    // draw starting pixel
    draw_pixel(x, y, red, green, blue);
    doer(x, y);

    int i; // so we can use it after the loop

    // step 'run' many steps along the stepper axis
    for (i = 0; i < *run; i++) {
        fprintf(stderr, "Step %d\n", i+1);
        // advance stepper
        *stepper += *step;
        // advance "error"
        error += increment;

        // bucket is full
        if (error >= 0) {
            // advance the bumper
            *bumper += *bump;
            // "reset" the "error"
            error -= drain;
        }

        draw_pixel(x, y, red, green, blue);

        doer(x, y);

        /* check for valid space here */
        if (! checker(x, y)) {
            break;
        }

        // cross-fade color
        green -= color_step;
        red += color_step;
    }

    if (i == *run) {
        // We made it. Doesn't matter if the final square
        // was inaccessible, because most of the time it'll
        // be occupied by something.
        return true;
    }

    // re-draw last pixel to mark where we hit something
    // this would not be in the actual code
    draw_pixel(x, y, 0.0, 0.0, 1.0);
    doer(x,y);
    return false;
}

///////////////////////
// GL Rendering Code //
///////////////////////

void display() {
    glClear(GL_COLOR_BUFFER_BIT);
    // these values should be multiples of PIXELS_PER_SQUARE

    //braise(8, 22, 4, 2);
    //braise(18, 14, 11, 46);
    //braise(4, 2, 33, 8);
    //braise(40, 40, 41, 20);

    // rays out from center
    braise(22, 28, 11, 33); // up & left
    braise(29, 34, 41, 42); // up & right
    braise(26, 28, 37, 11); // down & right
    braise(18, 18, 3, 8); // down & left

    // opposite directions
    braise(5, 45, 23, 45); // left to right
    braise(23, 43, 5, 43); // right to left
    braise(45, 45, 45, 25); // top to bottom
    braise(43, 25, 43, 45); // bottom to top

    glFlush();
}

void myinit() {
    glClearColor(1.0, 1.0, 1.0, 1.0);
    glPointSize((float)PIXELS_PER_SQUARE);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, 499.0, 0.0, 499.0);
}

void main(int argc, char** argv) {
/* standard GLUT initialization */
    glutInit(&argc,argv);
    glutInitWindowSize(WINDOW_WIDTH,WINDOW_HEIGHT);
    glutInitWindowPosition(0,0);
    glutCreateWindow("Braising Hams");
    glutDisplayFunc(display); /* display callback invoked when window opened */

    myinit(); /* set attributes */

    glutMainLoop(); /* enter event loop */
}
