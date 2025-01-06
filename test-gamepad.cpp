#include "pico-joystick.h"

void configure_test_button(GPInput *button) {
    button->set_pullup_down();
    button->set_is_inverted();
    button->set_debounce(1);
}

static void threads_main(int argc, char **argv) {
    Button *test_button = new Button(28, "test-button");
    configure_test_button(test_button);

    pico_joystick_on_boot(test_button, 28);

    test_button->set_button_id(0);

    pico_joystick_start("test gamepad", "test-gamepad");
}
    
    
int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}

