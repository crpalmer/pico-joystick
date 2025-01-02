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
    pico_joystick_on_boot("pico-joystick", WAKE_GPIO, WAKE_GPIO, 21, 27);
    new Button(31, 2, "joy1");
    new Button(30, 3, "joy2");
    new Button(29, 4, "joy3");
    new Button(28, 5, "joy4");
    new Button(27, 6, "little1");
    new Button(26, 14, "little2");
    new Button(25, 17, "little3");
    new Button(24, 9, "blue");
    new Button(23, 28, "red");
    pico_joystick_start("Pico Joystick");
}
    
    
int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}

