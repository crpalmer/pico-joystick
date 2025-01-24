#include "pi.h"
#include <math.h>
#include "i2c.h"
#include "bluetooth/bluetooth.h"
#include "gamepad.h"
#include "pico-joystick.h"
#include "pi-threads.h"
#include "random-utils.h"
#include "time-utils.h"

#define STATUS_REG 0x0B

#define I2C_BUS		1
#define I2C_SDA		2
#define I2C_SCL		3

static void ensure_magnet(int i2c) {
    uint8_t last_status = 0;
    while (1) {
	uint8_t status;

	if (i2c_read_byte(i2c, STATUS_REG, &status) < 0) {
	    fprintf(stderr, "Failed to read the status register\n");
	    assert(0);
	}

	bool md = (status & 0x20) != 0;
	bool ml = (status & 0x10) != 0;
	bool mh = (status & 0x08) != 0;
	if (md) {
	    fprintf(stderr, "Magnet detected.\n");
	    break;
	} else if (mh) {
	    if (status != last_status) fprintf(stderr, "Magnet is too close to the sensor\n");
	} else if (ml) {
	    if (status != last_status) fprintf(stderr, "Magnet is too far from the sensor\n");
	} else {
	    if (status != last_status) fprintf(stderr, "Unexpected status response: 0x%02x.\n", status);
	}
	last_status = status;
	ms_sleep(100);
    }
}

static double read_angle(int i2c) {
    uint8_t low_byte, high_nibble;
    static uint16_t last_value = 0;

    i2c_read_byte(i2c, 0x0c, &high_nibble);
    i2c_read_byte(i2c, 0x0d, &low_byte);

    uint16_t value = (high_nibble << 8) | low_byte;
    if (value >= 4096) {
	value = last_value;
    } else if (-2 <= (value - last_value) && (value - last_value) <= 2) {
	value = last_value;
    } else {
        last_value = value;
    }
    return value / 4095.0;
}

static void threads_main(int argc, char **argv) {
    int i2c;

#if 1
    bluetooth_init();
    hid_init();
#else
    pico_joystick_boot();
#endif

    i2c_init_bus(I2C_BUS, I2C_SDA, I2C_SCL);
    if ((i2c = i2c_open(I2C_BUS, 0x36)) < 0) {
	fprintf(stderr, "Failed to open the i2c device.\n");
	assert(0);
    }

    GPInput *button = new GPInput(5);
    button->set_pullup_up();

    Mouse *mouse = new Mouse();
    HIDButtons *buttons = new HIDButtons(mouse, 1, 1);
    mouse->add_hid_page(buttons);
    HIDSpinner *spinner = new HIDSpinner(mouse);
    mouse->add_hid_page(spinner);

    mouse->initialize("spinner");
    bluetooth_start(hid_subclass_mouse, "Pico Spinner");

    ensure_magnet(i2c);

    while (1) {
#if 0
static double position = 0;

	ms_sleep(990);

	position += random_number() * 0.5 - 0.25;
	if (position < 0) position = 1 + position;
	if (position > 1) position = position - 1;

	spinner->set_position(position);
	//buttons->set_button(1, random_number_in_range(0, 1));
#else
	spinner->set_position(read_angle(i2c));
	buttons->set_button(1, button->get());
#endif
	ms_sleep(10);

    }
}

int main(int argc, char **argv) {
    pi_init_with_threads(threads_main, argc, argv);
}
