#include <string.h>
#include "hardware/watchdog.h"
#include "pi.h"
#include "bluetooth/bluetooth.h"
#include "bluetooth/hid.h"
#include "deep-sleep.h"
#include "gp-output.h"
#include "net-console.h"
#include "net-listener.h"
#include "stdin-reader.h"
#include "stdout-writer.h"
#include "time-utils.h"
#include "wifi.h"
#include "pico-joystick.h"

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
       }
    }

private:
   struct timespec last_press;

   const int SLEEP_CHECK_MS = 60*1000;
   const int SLEEP_MS = 15 * 60*1000;
};

class GamePad : public HID {
public:
    GamePad(Sleeper *sleeper, const char *name, GPOutput *connected_led, uint8_t *descriptor, uint16_t hid_descriptor_len, uint8_t subclass) : HID(name, descriptor, hid_descriptor_len, subclass), sleeper(sleeper), connected_led(connected_led) {
	if (connected_led) connected_led->off();
    }

    void can_send_now() override {
	sleeper->prod();
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
	if (connected_led) connected_led->on();
    }

    void on_disconnect() override {
	if (connected_led) connected_led->off();
    }

private:
   class Sleeper *sleeper;
   uint8_t report[5] = { 0xa1 };
   uint8_t *state = &report[1];
   GPOutput *connected_led;
};

static class GamePad *gp;

Button::Button(int gpio, const char *name) : GPInput(gpio), PiThread(name) { 
    start();
    set_notifier(this);
}

void Button::on_change(void) {
    resume_from_isr();
}

void Button::main() {
    while (1) {
	int count = 0;
	bool this_value = get();

	if (debounce_ms == 0) {
	    do {
	        bool check_value = get();
	        if (this_value == check_value) {
		    count++;
	        } else {
		    this_value = check_value;
		    count = 0;
	        }
	    } while (count < 100);
	}

	if (this_value != last_value) {
	    if (button_id >= 0) gp->set_button(button_id, this_value);
	    last_value = this_value;
	}
	pause();
    }
}

class ConsoleThread : public NetConsole, public PiThread {
public:
    ConsoleThread(Reader *r, Writer *w) : NetConsole(r, w), PiThread("console") { start(); }
    ConsoleThread(int fd) : NetConsole(fd), PiThread("console") { start(); }

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

class NetListenerThread : NetListener {
public:
    NetListenerThread(uint16_t port) : NetListener(port) { start(); }

    void accepted(int fd) {
        new ConsoleThread(fd);
    }
};

static bool has_wifi = false;

class GamePad *pico_joystick_on_boot(const char *hostname, GPInput *bootloader_button, int wakeup_gpio, GPOutput *power_led, GPOutput *bluetooth_led, GPInput *wifi_enable_button) {
    const int BOOTLOADER_HOLD_MS = 100;

    ms_sleep(1000);	// This seems to be important to let the bluetooth stack and the threads library both get started

    if (power_led) power_led->off();
    if (bluetooth_led) bluetooth_led->off();

    printf("Checking for bootloader request\n");
    struct timespec start;
    nano_gettime(&start);

    while (bootloader_button->get()) {
	if (nano_elapsed_ms_now(&start) >= BOOTLOADER_HOLD_MS) {
	    pi_reboot_bootloader();
	}
    }

    printf("No bootloader request, checking for deep sleep\n");

    if (watchdog_caused_reboot()) {
	printf("Going to sleep on gpio %d...\n", wakeup_gpio);
	pico_enter_deep_sleep_until(wakeup_gpio);
    } else {
        printf("Starting\n");
    }

    if (power_led) power_led->on();

    has_wifi = wifi_enable_button != NULL && wifi_enable_button->get();
    if (has_wifi) wifi_init(hostname);

    bluetooth_init();
    hid_init();

    Sleeper *sleeper = new Sleeper();
    gp = new GamePad(sleeper, "gamepad", bluetooth_led, (uint8_t *) profile_data, sizeof(profile_data), (uint8_t) 0x580);

    return gp;
}

void pico_joystick_start(const char *bluetooth_name) {
    bluetooth_start_gamepad(bluetooth_name);
    if (has_wifi) {
        wifi_wait_for_connection();
        new NetListenerThread(4567);
    }
}
