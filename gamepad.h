#ifndef __GAMEPAD_H__
#define __GAMEPAD_H__

#include "writer.h"
#include "bluetooth/hid.h"

class Gamepad : public HID {
public:
    Gamepad(const char *name) {
	initialize(name, descriptor, hid_descriptor_len, subclass);
    }

    void can_send_now() override {
	send_report(report, 5);
    }

    virtual void set_button(int button, bool value) {
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

    void dump_state(Writer *w) {
	w->write_str("buttons pressed:");
	for (int button = 0; button < 32; button++) {
	    if (is_button_pressed(button)) w->printf(" %d", button);
	}
	w->write_str("\n");
    }

private:
    uint8_t report[5] = { 0xa1 };
    uint8_t *state = &report[1];

    static const int subclass = 0x580;
    static const int hid_descriptor_len = 23;
    const uint8_t descriptor[hid_descriptor_len] = {
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
};

#endif
