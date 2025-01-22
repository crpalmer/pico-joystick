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
#include "gamepad.h"
#include "pico-joystick.h"

class SleepThenReboot : public DeepSleeper {
public:
    SleepThenReboot(int wakeup_gpio) : DeepSleeper(wakeup_gpio, 1) {
    }

    void post_sleep() override {
        pi_reboot();
    }
};

Button::Button(int gpio, const char *name) : GPInput(gpio), PiThread(name) { 
}

void Button::set_button_id(HIDButtons *buttons, int button_id) {
    assert(this->button_id < 0);
    assert(button_id >= 0);
    assert(buttons);
    this->buttons = buttons;
    this->button_id = button_id;
    start(3);
}

void Button::on_change(void) {
    resume_from_isr();
}

void Button::main() {
    // Set the notifier on the core we'll be running on
    set_notifier(this);

    while (1) {
	bool this_value = get();
	if (this_value != last_value) {
	    buttons->set_button(button_id, this_value);
	    last_value = this_value;
	}
	pause();
    }
}

class ConsoleThread : public ThreadsConsole, public PiThread {
public:
    ConsoleThread(Reader *r, Writer *w, const char *name = "console") : ThreadsConsole(r, w), PiThread("console") {
	start();
    }

    void main(void) {
	ThreadsConsole::main();
    }

    void process_cmd(const char *cmd) override {
        ThreadsConsole::process_cmd(cmd);
    }

    void usage() override {
	ThreadsConsole::usage();
	write_str("usage: <button #> <0|1> | threads\n");
    }
};

class NetConsoleThread : public NetConsole, public PiThread {
public:
    NetConsoleThread(int fd) : NetConsole(fd), PiThread("console") { start(); }

    void main() {
	NetConsole::main();
    }

    void process_cmd(const char *cmd) override {
        NetConsole::process_cmd(cmd);
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
        new NetConsoleThread(fd);
    }
};

class StartWifiThread : public PiThread {
public:
    StartWifiThread(const char *hostname) : PiThread("start-wifi"), hostname(hostname) {
	start();
    }

    void main(void) override {
	wifi_init(hostname);
        wifi_wait_for_connection();
        new NetListenerThread(4567);
    }

private:
    const char *hostname;
};

void pico_joystick_go_to_sleep() {
    watchdog_enable(1, 0);
}

void pico_joystick_boot(Input *bootloader_button, int wakeup_gpio, Input *wifi_enable_button, const char *hostname) {
    const int BOOTLOADER_HOLD_MS = 100;

    printf("Checking for bootloader request\n");
    struct timespec start;
    nano_gettime(&start);

    while (bootloader_button && bootloader_button->get()) {
	if (nano_elapsed_ms_now(&start) >= BOOTLOADER_HOLD_MS) {
	    pi_reboot_bootloader();
	}
    }

    if (watchdog_enable_caused_reboot() && wakeup_gpio >= 0) {
	printf("Going to sleep in 1ms\n"); fflush(stdout);
	new SleepThenReboot(wakeup_gpio);
	ms_sleep(100);
    }

    printf("Starting\n");

    new ConsoleThread(new StdinReader(), new StdoutWriter());

    bool has_wifi = false;
    if (! wifi_enable_button) has_wifi = (hostname != NULL);
    else has_wifi = wifi_enable_button->get();

    bluetooth_init();
    hid_init();

    if (has_wifi) new StartWifiThread(hostname);
}
