#ifndef __GAMEPAD_H__
#define __GAMEPAD_H__

#include "pi.h"
#include "mem.h"
#include "writer.h"
#include "bluetooth/hid.h"
#include "memory.h"

class HIDButtons {
public:
    HIDButtons(HID *hid, int first_button_id = 0, int last_button_id = 31) {
	n_buttons = (last_button_id - first_button_id + 1);
	n_buttons = (n_buttons + 7) / 8 * 8;

	state = (uint8_t *) fatal_malloc(n_buttons / 8);

	button_range[0] = first_button_id;
	button_range[1] = ((last_button_id - first_button_id + 1) + 7) / 8 + first_button_id;
   }

   ~HIDButtons() {
	fatal_free(state);
   }

   int add_descriptor(uint8_t *descriptor) {
	int i = 0;
	descriptor[i++] = 0x05;
	descriptor[i++] = 0x09; // USAGE_PAGE (Button)
	descriptor[i++] = 0x19;
	descriptor[i++] = 0x01; // USAGE_MINIMUM (Button 1)
	descriptor[i++] = 0x29;
	descriptor[i++] = 0x80; // USAGE_MAXIMUM (Button 32)
	descriptor[i++] = 0x15;
	descriptor[i++] = 0x00; // LOGICAL_MINIMUM (0)
	descriptor[i++] = 0x25;
	descriptor[i++] = 0x01; // LOGICAL_MAXIMUM (1)
	descriptor[i++] = 0x95;
	descriptor[i++] = 0x20; // REPORT_COUNT (32)
	descriptor[i++] = 0x75;
	descriptor[i++] = 0x01; // REPORT_SIZE (1)
	descriptor[i++] = 0x81;
	descriptor[i++] = 0x02; // INPUT (Data,Var,Abs)

	return i;
    }

    int get_report_size() {
	return n_buttons / 8;
    }

    void fill_report(uint8_t *buf) {
	memcpy(buf, state, get_report_size());
    }

    void set_button(int id, bool value) {
	//assert(id >= button_range[0] && id <= button_range[1]);

	int byte = id/8;
	int bit = id%8;

	if (value) state[byte] |= (1 << bit);
	else state[byte] &= ~(1 << bit);

	hid->request_can_send_now();
    }

private:
    HID *hid;
    uint8_t *state;
    int n_buttons;
    int button_range[2];
};

class Gamepad : public HID {
public:
    Gamepad() {
	buttons = new HIDButtons(this);
    }

    HIDButtons *get_buttons() { return buttons; }

    void initialize(const char *name) {
	int descriptor_len = 0;
	descriptor[descriptor_len++] = 0x05;
	descriptor[descriptor_len++] = 0x01;                    // USAGE_PAGE (Generic Desktop)
	descriptor[descriptor_len++] = 0x09;
	descriptor[descriptor_len++] = 0x04;                    // USAGE (Gamepad)
	descriptor[descriptor_len++] = 0xa1;
	descriptor[descriptor_len++] = 0x01;                    // COLLECTION (Application)
	descriptor_len += buttons->add_descriptor(&descriptor[descriptor_len]);
	descriptor[descriptor_len++] = 0xc0;                    // END_COLLECTION

	HID::initialize(name, descriptor, descriptor_len, subclass);

	report_size = 1;
	report_size += buttons->get_report_size();

	report = (uint8_t *) fatal_malloc(sizeof(*report) * report_size);
	report[0] = 0xa1;
    }

    void can_send_now() override {
	buttons->fill_report(&report[1]);
	send_report(report, report_size);
    }

private:
    HIDButtons *buttons;
    int report_size;
    uint8_t *report;

    static const int subclass = 0x580;
    static const int max_descriptor_len = 1024;
    uint8_t descriptor[max_descriptor_len];
};

#endif
