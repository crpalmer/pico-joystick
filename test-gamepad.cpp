#include "pi.h"
#include "pico-adc.h"
#include "bluetooth/bluetooth.h"
#include "gamepad.h"
#include "pico-joystick.h"
#include "pi-threads.h"
#include "time-utils.h"

#include "random-utils.h"

Button *configure_test_button(Button *button) {
    button->set_pullup_up();
    return button;
}

static void threads_main(int argc, char **argv) {
    pico_joystick_boot();

    Gamepad *gp = new Gamepad();
    HIDButtons *buttons = new HIDButtons(gp, 1, 8);
    HIDXY *xy = new HIDXY(gp);

    gp->add_hid_page(xy);
    gp->add_hid_page(buttons);

    configure_test_button(new Button(11, "test-button 1"))->set_button_id(buttons, 1);
    configure_test_button(new Button(12, "test-button 2"))->set_button_id(buttons, 2);
    configure_test_button(new Button(10, "joystick button"))->set_button_id(buttons, 3);

    gp->initialize("Test Gamepad");
    bluetooth_start_gamepad("Test Gamepad");

    ADC *adc = new PicoADC();

    while (1) {
	double x = adc->read_percentage(0);
	double y = adc->read_percentage(1);
	xy->move(x, y);
    }
}
    
int main(int argc, char **argv) {
    pi_init_with_threads(threads_main, argc, argv);
}
