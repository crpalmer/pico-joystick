#include "pico-joystick.h"
#include "time-utils.h"

Button *configure_test_button(Button *button) {
    button->set_pullup_down();
    button->set_is_inverted();
    button->set_debounce(1);
    return button;
}

static void threads_main(int argc, char **argv) {
    pico_joystick_run("Test Gamepad");

    Button *test_button = configure_test_button(new Button(28, "test-button"));
    test_button->set_button_id(0);
}
    
    
int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}

