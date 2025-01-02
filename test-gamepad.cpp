#include "pico-joystick.h"

void configure_button(GPInput *button) {
    if (button->get_gpio() == 28) {
	button->set_pullup_down();
	button->set_is_inverted();
    } else {
        button->set_pullup_up();
    }
    button->set_debounce(1);
}

static void threads_main(int argc, char **argv) {
    pico_joystick_on_boot("test-gamepad", 28, 0, 27, 26);
    new Button(0, 28, "test-button");
    pico_joystick_start("test gamepad");
}
    
    
int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}

