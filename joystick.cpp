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
	connected = true;
    }

    void on_disconnect() override {
	led->off();
	Gamepad::on_disconnect();
	connected = false;
    }

    void wait_connected() {
	while (! connected) ms_sleep(1000);
    }

private:
    Output *led;
    DeepSleeper *sleeper;
    bool connected = false;
};

static struct {
    int gpio;
    bool pull_down;
    const char *name;
    GPInput *input;
} buttons[] = {
    { 2, false, "up" },
    { 3, false, "down" },
    { 4, false, "left" },
    { 5, false, "right" },
    {10, true,  "b1" },
    {11, false, "b2" },
    {12, false, "b3" },
    { 6, false, "start" },
    { 7, false, "select" },
    { 8, false, "meta" },
};

static const int n_buttons = sizeof(buttons) / sizeof(*buttons);

static GPInput *get_button(const char *name) {
    for (int i = 0; i < n_buttons; i++) {
	if (strcmp(buttons[i].name, name) == 0) return buttons[i].input;
    }
    return NULL;
}

static void threads_main(int argc, char **argv) {
    for (int i = 0; i < n_buttons; i++) {
	buttons[i].input = new GPInput(buttons[i].gpio);
	if (buttons[i].pull_down) {
	    buttons[i].input->set_pullup_down();
	    buttons[i].input->set_is_inverted();
	} else {
	    buttons[i].input->set_pullup_up();
	}
    }

    GPInput *start  = get_button("start");
    GPInput *b1     = get_button("b1");

    pico_joystick_boot(b1, 10, start, "joystick");

    GPOutput *power_led = new GPOutput(19);
    power_led->on();

    Joystick *joystick = new Joystick(new GPOutput(18), new Sleeper());
    HIDButtons *hid_buttons = new HIDButtons(joystick, 1, n_buttons+1);
    joystick->add_hid_page(hid_buttons);
    joystick->initialize("Test Gamepad");
    bluetooth_start_gamepad("Pico Joystick");

    while (1) {
	joystick->wait_connected();
	hid_buttons->begin_transaction();
	for (int i = 0; i < n_buttons; i++) {
	    hid_buttons->set_button(i+1, buttons[i].input->get());
	}
	hid_buttons->end_transaction();
    }
}

int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}

