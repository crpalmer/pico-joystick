#include "pi.h"
#include "bluetooth/bluetooth.h"
#include "deep-sleep.h"
#include "gamepad.h"
#include "pico-joystick.h"

class Sleeper : public DeepSleeper {
public:
    Sleeper() : DeepSleeper(-1, 10*60*1000) {
    }

    void pre_sleep(void) override {
	pico_joystick_go_to_sleep();
    }
};

class Joystick : public Gamepad {
public:
    Joystick(Output *led, DeepSleeper *sleeper) : led(led), sleeper(sleeper) {
    }

    void can_send_now() override {
	sleeper->prod();
	Gamepad::can_send_now();
    }

    void on_connect() override {
	sleeper->prod();
	Gamepad::on_connect();
	led->on();
    }

    void on_disconnect() override {
	led->off();
	Gamepad::on_disconnect();
    }

private:
    Output *led;
    DeepSleeper *sleeper;
};

static Button *configure_wakeup_game_button(Button *button) {
    button->set_pullup_down();
    button->set_is_inverted();
    return button;
}

static Button *configure_ui_button(Button *button) {
    button->set_pullup_up();
    button->set_debounce(10);
    return button;
}

static Button *configure_game_button(Button *button) {
    button->set_pullup_up();
    return button;
}

static void threads_main(int argc, char **argv) {
    Button *start  = configure_ui_button(new Button(6, "start"));
    Button *b1     = configure_wakeup_game_button(new Button(10, "button-1"));

    pico_joystick_boot(b1, 10, start, "joystick");

    GPOutput *power_led = new GPOutput(19);
    power_led->on();

    Gamepad *gp = new Joystick(new GPOutput(18), new Sleeper());
    HIDButtons *buttons = new HIDButtons(gp);
    gp->add_hid_page(buttons);
    gp->initialize("Test Gamepad");

    Button *up     = configure_game_button(new Button(2, "up"));
    Button *down   = configure_game_button(new Button(3, "down"));
    Button *left   = configure_game_button(new Button(4, "left"));
    Button *right  = configure_game_button(new Button(5, "right"));
    Button *select = configure_ui_button  (new Button(7, "select"));
    Button *meta   = configure_ui_button  (new Button(8, "meta"));
    Button *b2     = configure_game_button(new Button(11, "button-2"));
    Button *b3     = configure_game_button(new Button(12, "button-3"));

    down->set_button_id(buttons, 31);
    up->set_button_id(buttons, 30);
    right->set_button_id(buttons, 29);
    left->set_button_id(buttons, 28);
    meta->set_button_id(buttons, 27);
    start->set_button_id(buttons, 26);
    select->set_button_id(buttons, 25);
    b2->set_button_id(buttons, 24);
    b1->set_button_id(buttons, 23);
    b3->set_button_id(buttons, 22);

    bluetooth_start_gamepad("Pico Joystick");
    new Sleeper();
}
    
int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}

