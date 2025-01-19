#include "pi.h"
#include "bluetooth/bluetooth.h"
#include "gamepad.h"
#include "pico-joystick.h"
#include "pi-threads.h"
#include "time-utils.h"

class TestGamepad : public Gamepad {
public:
    TestGamepad() : Gamepad("Test Gamepad") {
    }

    void set_button(int button_id, bool value) override {
	printf("%d %d\n", button_id, value);
	Gamepad::set_button(button_id, value);
    }
};

Button *configure_test_button(Button *button) {
    button->set_pullup_up();
    return button;
}

static void threads_main(int argc, char **argv) {
    pico_joystick_boot();

    Gamepad *gp = new TestGamepad();

    configure_test_button(new Button(11, "test-button 1"))->set_button_id(gp, 1);
    configure_test_button(new Button(12, "test-button 2"))->set_button_id(gp, 2);

    bluetooth_start_gamepad("Test Gamepad");
}
    
int main(int argc, char **argv) {
    pi_init_with_threads(threads_main, argc, argv);
}
