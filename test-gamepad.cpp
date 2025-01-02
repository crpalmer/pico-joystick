#include "pico-joystick.h"

static void threads_main(int argc, char **argv) {
    pico_joystick_on_boot("test-gamepad", 28, 0, 27);
    new Button(0, 28, "test-button");
    pico_joystick_start("test gamepad");
}
    
    
int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}

