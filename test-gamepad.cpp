#include "pi.h"
#include "bluetooth/bluetooth.h"
#include "gamepad.h"
#include "pico-joystick.h"
#include "pi-threads.h"
#include "time-utils.h"

Button *configure_test_button(Button *button) {
    button->set_pullup_up();
    return button;
}

static void threads_main(int argc, char **argv) {
    pico_joystick_boot();

    Gamepad *gp = new Gamepad();
    HIDButtons *buttons = new HIDButtons(gp);
    gp->add_hid_page(buttons);

    configure_test_button(new Button(11, "test-button 1"))->set_button_id(buttons, 1);
    configure_test_button(new Button(12, "test-button 2"))->set_button_id(buttons, 2);

    gp->initialize("Test Gamepad");
    bluetooth_start_gamepad("Test Gamepad");
}
    
int main(int argc, char **argv) {
    pi_init_with_threads(threads_main, argc, argv);
}
