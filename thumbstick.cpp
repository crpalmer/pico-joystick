#include "pi.h"
#include "bluetooth/bluetooth.h"
#include "deep-sleep.h"
#include "gamepad.h"
#include "pico-adc.h"
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
    Joystick(DeepSleeper *sleeper) : sleeper(sleeper) {
    }

    void can_send_now() override {
	sleeper->prod();
	Gamepad::can_send_now();
    }

    void on_connect() override {
	sleeper->prod();
	Gamepad::on_connect();
	connected = true;
    }

    void on_disconnect() override {
	Gamepad::on_disconnect();
	connected = false;
    }

    void wait_connected() {
	while (! connected) ms_sleep(1000);
    }

private:
    DeepSleeper *sleeper;
    bool connected = false;
};

static struct {
    int gpio;
    const char *name;
    GPInput *input;
} buttons[] = {
    {-1, "up" },
    {-1, "down" },
    {-1, "left" },
    {-1, "right" },
    {13, "b1" },
    { 9, "b2" },
    {-1, "b3" },
    { 2, "start" },
    { 5, "select" },
    {-1, "meta" },
    {26, "thumb-switch" },
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
	if (buttons[i].gpio >= 0) {
	    buttons[i].input = new GPInput(buttons[i].gpio);
	    buttons[i].input->set_pullup_up();
	}
    }

    ADC *adc = new PicoADC();

    GPInput *start  = get_button("start");
    GPInput *b1     = get_button("b1");

    pico_joystick_boot(b1, 13, start, "joystick");

    Joystick *joystick = new Joystick(new Sleeper());
    HIDButtons *hid_buttons = new HIDButtons(joystick, 1, n_buttons+1);
    joystick->add_hid_page(hid_buttons);
    joystick->initialize("Pico Thumbstick");
    bluetooth_start_gamepad("Pico Thumbstick");

    while (1) {
	joystick->wait_connected();
	hid_buttons->begin_transaction();

	double x = adc->read_percentage(2);
	double y = adc->read_percentage(1);

	for (int i = 1; i <= 4; i++) hid_buttons->set_button(i, false);

	if (y > 0.75) hid_buttons->set_button(1, true);
	if (y < 0.25) hid_buttons->set_button(2, true);
	if (x > 0.75) hid_buttons->set_button(3, true);
	if (x < 0.25) hid_buttons->set_button(4, true);

	for (int i = 0; i < n_buttons; i++) {
	    if (buttons[i].gpio < 0) continue;
	    hid_buttons->set_button(i+1, buttons[i].input->get());
	}

	hid_buttons->end_transaction();
    }
}

int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}
