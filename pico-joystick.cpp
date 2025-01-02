#include <string.h>
#include "hardware/watchdog.h"
#include "pi.h"
#include "bluetooth/bluetooth.h"
#include "bluetooth/hid.h"
#include "deep-sleep.h"
#include "gp-input.h"
#include "gp-output.h"
#include "net-console.h"
#include "net-listener.h"
#include "pi-threads.h"
#include "time-utils.h"
#include "wifi.h"

const uint8_t profile_data[] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x04,                    // USAGE (Gamepad)
    0xa1, 0x01,                    // COLLECTION (Application)

    0x05, 0x09, // USAGE_PAGE (Button)
    0x19, 0x01, // USAGE_MINIMUM (Button 1)
    0x29, 0x80, // USAGE_MAXIMUM (Button 32)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x95, 0x20, // REPORT_COUNT (32)
    0x75, 0x01, // REPORT_SIZE (1)
    0x81, 0x02, // INPUT (Data,Var,Abs)

    0xc0                           // END_COLLECTION
};

class GamePad;

static void configure_button(GPInput *button) {
    button->set_pullup_up();
    button->set_debounce(1);
}

class Button : public GPInput, public InputNotifier, PiThread {
public:
    Button(class GamePad *gp, int button_id, int gpio, const char *name = "button") : GPInput(gpio), PiThread(name), gp(gp), button_id(button_id) {
	configure_button(this);
	set_notifier(this);
	start();
    }

    void main(void) override;
    void on_change(void) override;

private:
    class GamePad *gp;
    int last_value = -1;
    int button_id;
};

class Sleeper : public PiThread {
public:
    Sleeper() : PiThread("sleeper") {
	prod();
	start();
    }

    void prod() {
	nano_gettime(&last_press);
    }

    void main(void) {
	while (1) {
	    ms_sleep(SLEEP_CHECK_MS);
	    if (nano_elapsed_ms_now(&last_press) >= SLEEP_MS) {
		watchdog_reboot(0, 0, 0);
	    }
printf("%d\n", (nano_elapsed_ms_now(&last_press) + 500) / 1000);
	}
    }

private:
   struct timespec last_press;

   const int SLEEP_CHECK_MS = 60*1000;
   const int SLEEP_MS = 15 * 60*1000;
};

class GamePad : public HID {
public:
    GamePad(Sleeper *sleeper, const char *name, uint8_t *descriptor, uint16_t hid_descriptor_len, uint8_t subclass) : HID(name, descriptor, hid_descriptor_len, subclass), sleeper(sleeper) {
	request_can_send_now();
	connected_led = new GPOutput(27);
	connected_led->off();
    }

    void can_send_now() override {
	sleeper->prod();
	consoles_printf("Can send now!\n");
	send_report(report, 5);
    }

    void set_button(int button, bool value) {
	int byte = button/8;
	int bit = button%8;

	if (value) state[byte] |= (1 << bit);
	else state[byte] &= ~(1 << bit);

	request_can_send_now();
    }

    bool is_button_pressed(int button) {
	int byte = button/8;
	int bit = button%8;

	return (state[byte] & (1 << bit)) != 0;
    }

    void on_connect() override {
	connected_led->on();
    }

    void on_disconnect() override {
	connected_led->off();
    }

private:
   class Sleeper *sleeper;
   uint8_t report[5] = { 0xa1 };
   uint8_t *state = &report[1];
   GPOutput *connected_led;
};

void Button::on_change(void) {
    resume_from_isr();
}

void Button::main() {
    while (1) {
	bool this_value = get();
	if (this_value != last_value) {
	    gp->set_button(button_id, this_value);
	    last_value = this_value;
	}
	pause();
    }
}

static class GamePad *gp;

class TestingThread : public NetConsole, public PiThread {
public:
    TestingThread(Reader *r, Writer *w) : NetConsole(r, w), PiThread("testing-thread") { start(); }
    TestingThread(int fd) : NetConsole(fd), PiThread("testing-thread") { start(); }

    void main() {
	Console::main();
    }

    void process_cmd(const char *cmd) override {
	int button, value;

	if (sscanf(cmd, "%d %d", &button, &value) == 2) {
	    gp->set_button(button, value);
	} else if (is_command(cmd, "state")) {
	    write_str("buttons pressed:");
	    for (int button = 0; button < 32; button++) {
		if (gp->is_button_pressed(button)) printf(" %d", button);
	    }
	    write_str("\n");
	} else {
            NetConsole::process_cmd(cmd);
        }
    }

    void usage() override {
	NetConsole::usage();
	write_str("usage: <button #> <0|1> | threads\n");
    }
};

class NetThread : NetListener {
public:
    NetThread(uint16_t port) : NetListener(port) { start(); }

    void accepted(int fd) {
        new TestingThread(fd);
    }
};

static void check_bootloader_boot() {
    const int BOOTLOADER_HOLD_GPIO = 6;
    const int BOOTLOADER_HOLD_MS = 1000;

    GPInput *bootloader_gpio = new GPInput(BOOTLOADER_HOLD_GPIO);
    configure_button(bootloader_gpio);

    struct timespec start;
    nano_gettime(&start);

    while (bootloader_gpio->get()) {
	if (nano_elapsed_ms_now(&start) >= BOOTLOADER_HOLD_MS) {
	    pi_reboot_bootloader();
	}
    }
}

static void threads_main(int argc, char **argv) {
    const char *name = "Pico GamePad";
    const int WAKEUP_GPIO = 28;
    const int POWER_LED_GPIO = 26;

    check_bootloader_boot();

    if (false && watchdog_caused_reboot()) {
	printf("Going to sleep...\n");
	pico_enter_deep_sleep_until(WAKEUP_GPIO);
    } else {
        printf("Starting\n");
    }

    GPOutput *power_led = new GPOutput(POWER_LED_GPIO);
    power_led->on();

    Sleeper *sleeper = new Sleeper();

    wifi_init(name);
    bluetooth_init();
    hid_init();
    gp = new GamePad(sleeper, "gamepad", (uint8_t *) profile_data, sizeof(profile_data), (uint8_t) 0x580);
    new Button(gp, 0, 28, "test-button");
    new Button(gp, 1, 2, "joy1");
    new Button(gp, 2, 3, "joy2");
    new Button(gp, 3, 4, "joy3");
    new Button(gp, 4, 5, "joy4");
    new Button(gp, 5, 6, "button1");
    new Button(gp, 6, 9, "button2");
    new Button(gp, 7, 13, "button3");
    new Button(gp, 8, 17, "button4");
    bluetooth_start_gamepad(name);
    wifi_wait_for_connection();
    new NetThread(4567);
}

int main(int argc, char **argv) {
    pi_init_with_threads(threads_main, argc, argv);
}
