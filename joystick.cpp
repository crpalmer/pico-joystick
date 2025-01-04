#include "pico-joystick.h"

#define WAKE_GPIO 28

void configure_button(GPInput *button) {
    if (button->get_gpio() == WAKE_GPIO) {
        button->set_pullup_down();
        button->set_is_inverted();
    } else {
        button->set_pullup_up();
    }
    button->set_debounce(1);
}

static void threads_main(int argc, char **argv) {
    pico_joystick_on_boot("pico-joystick", WAKE_GPIO, WAKE_GPIO, 21, 27, 9);
    new Button(31, 2, "down");
    new Button(30, 3, "up");
    new Button(29, 4, "right");
    new Button(28, 5, "left");
    new Button(27, 6, "meta");
    new Button(26, 14, "start");
    new Button(25, 17, "select");
    new Button(24, 9, "button-2");
    new Button(23, 28, "button-1");
    new Button(22, 27, "button-3");
    pico_joystick_start("Pico Joystick");
}
    
int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}

