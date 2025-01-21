#include "pi.h"
#include <math.h>
#include "pico-adc.h"
#include "bluetooth/bluetooth.h"
#include "gamepad.h"
#include "pico-joystick.h"
#include "pi-threads.h"
#include "time-utils.h"

#include "random-utils.h"

#define ANALOG_JOYSTICK 0

Button *configure_test_button(Button *button) {
    button->set_pullup_up();
    return button;
}

static void threads_main(int argc, char **argv) {
    pico_joystick_boot();

    Gamepad *gp = new Gamepad();
    HIDButtons *buttons = new HIDButtons(gp, 1, 8);
    HIDXY *xy = new HIDXY(gp);

    GPInput *four_way = new GPInput(5);
    four_way->set_pullup_up();

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

#if ANALOG_JOYSTICK
	xy->move(x, y);
#else
	buttons->begin_transaction();
	buttons->set_button(4, false);
	buttons->set_button(5, false);
	buttons->set_button(6, false);
	buttons->set_button(7, false);
	
	double abs_x = fabs(x - 0.5);
	double abs_y = fabs(y - 0.5);

	if (four_way->get()) {
	    if (abs_x > 0.25 && abs_x > abs_y) {
		buttons->set_button(x > 0.5 ? 4 : 5, true);
	    } else if (abs_y > 0.25) {
		buttons->set_button(y > 0.5 ? 6 : 7, true);
	    }
	} else {
	    if (abs_x > 0.25) {
		buttons->set_button(x > 0.5 ? 4 : 5, true);
	    }
	    if (abs_y > 0.25) {
		buttons->set_button(y > 0.5 ? 6 : 7, true);
	    }
	}
	buttons->end_transaction();
#endif
    }
}
    
int main(int argc, char **argv) {
    pi_init_with_threads(threads_main, argc, argv);
}
