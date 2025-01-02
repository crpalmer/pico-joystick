#include "pico-joystick.h"

static void threads_main(int argc, char **argv) {
    pico_joystick_on_boot("pico-joystick", 14, 14, 27);
    new Button(0, 2, "joy1");
    new Button(1, 3, "joy2");
    new Button(2, 4, "joy3");
    new Button(3, 5, "joy4");
    new Button(4, 6, "button1");
    new Button(5, 9, "button2");
    new Button(6, 14, "button3");
    new Button(7, 17, "button4");
    pico_joystick_start("Pico Joystick");
}
    
    
int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}

