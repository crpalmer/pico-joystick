#ifndef __GAMEPAD_H__
#define __GAMEPAD_H__

#include "pi.h"
#include "mem.h"
#include "writer.h"
#include "bluetooth/hid.h"
#include "memory.h"
#include <list>

class HIDPage {
public:
    virtual int add_descriptor(uint8_t *descriptor) = 0;
    virtual int get_report_size() = 0;
    virtual void fill_report(uint8_t *report) = 0;
};

class HIDButtons : public HIDPage {
public:
    HIDButtons(HID *hid, int first_button_id = 0, int last_button_id = 31) : hid(hid) {
	n_buttons = (last_button_id - first_button_id + 1);
	n_buttons = (n_buttons + 7) / 8 * 8;

	state = (uint8_t *) fatal_malloc(n_buttons / 8);
	memset(state, 0, n_buttons / 8);

	button_range[0] = first_button_id;
	button_range[1] = last_button_id;
   }

   ~HIDButtons() {
	fatal_free(state);
   }

   int add_descriptor(uint8_t *descriptor) override {
	int i = 0;
	descriptor[i++] = 0x05;
	descriptor[i++] = 0x09; // USAGE_PAGE (Button)
	descriptor[i++] = 0x19;
	descriptor[i++] = button_range[0];    // USAGE_MINIMUM (Button #)
	descriptor[i++] = 0x29;
	descriptor[i++] = button_range[1];   // USAGE_MAXIMUM (Button #)
	descriptor[i++] = 0x15;
	descriptor[i++] = 0;    // LOGICAL_MINIMUM
	descriptor[i++] = 0x25;
	descriptor[i++] = 1;    // LOGICAL_MAXIMUM
	descriptor[i++] = 0x95;
	descriptor[i++] = n_buttons;   // REPORT_COUNT
	descriptor[i++] = 0x75;
	descriptor[i++] = 1;    // REPORT_SIZE
	descriptor[i++] = 0x81;
	descriptor[i++] = 0x02; // INPUT (Data,Var,Abs)

	return i;
    }

    int get_report_size() override {
	return n_buttons / 8;
    }

    void fill_report(uint8_t *buf) override {
	memcpy(buf, state, get_report_size());
    }

    void set_button(int id, bool value) {
	//assert(id >= button_range[0] && id <= button_range[1]);
	id -= button_range[0];

	int byte = id/8;
	int bit = id%8;

	uint8_t old_state = state[byte];
	if (value) state[byte] |= (1 << bit);
	else state[byte] &= ~(1 << bit);

	if (old_state != state[byte]) hid->request_can_send_now();
    }

private:
    HID *hid;
    uint8_t *state;
    int n_buttons;
    int button_range[2];
};

class HIDXY : public HIDPage {
public:
    HIDXY(HID *hid) : hid(hid) {
   }


   int add_descriptor(uint8_t *descriptor) override {
	int i = 0;
	descriptor[i++] = 0x05;
	descriptor[i++] = 0x01;	// USAGE_PAGE (Generic Desktop Controls)
	descriptor[i++] = 0x09;
	descriptor[i++] = 0x30; // USAGE (X)
	descriptor[i++] = 0x09;
	descriptor[i++] = 0x31; // USAGE (Y)
	descriptor[i++] = 0x15;
	descriptor[i++] = -127;    // LOGICAL_MINIMUM
	descriptor[i++] = 0x25;
	descriptor[i++] = 127;  // LOGICAL_MAXIMUM
	descriptor[i++] = 0x95;
	descriptor[i++] = 2;    // REPORT_COUNT
	descriptor[i++] = 0x75;
	descriptor[i++] = 8;    // REPORT_SIZE
	descriptor[i++] = 0x81;
	descriptor[i++] = 0x02; // INPUT (Data,Var,Abs)

	return i;
    }

    int get_report_size() override {
	return 2;
    }

    void fill_report(uint8_t *buf) override {
	buf[0] = x;
	buf[1] = y;
    }

    void move(double x_pct, double y_pct) {
	int8_t x = (x_pct * 254 - 127);
	int8_t y = (y_pct * 254 - 127);

	if (x != this->x || y != this->y) {
	    this->x = x;
	    this->y = y;
	    hid->request_can_send_now();
	} 
    }

private:
    HID *hid;
    int8_t x = 0;
    int8_t y = 0;
};

class Gamepad : public HID {
public:
    Gamepad() {
    }

    void add_hid_page(HIDPage *page) {
	hid_pages.push_back(page);
    }

    void initialize(const char *name) {
	int descriptor_len = 0;
	descriptor[descriptor_len++] = 0x05;
	descriptor[descriptor_len++] = 0x01; // USAGE_PAGE (Generic Desktop)
	descriptor[descriptor_len++] = 0x09;
	descriptor[descriptor_len++] = 0x04; // USAGE (Gamepad)
	descriptor[descriptor_len++] = 0xa1;
	descriptor[descriptor_len++] = 0x01; // COLLECTION (Application)
	descriptor[descriptor_len++] = 0xa1;
	descriptor[descriptor_len++] = 0x00; // COLLECTION (Physical)

	report_size = 1;
	for(auto page : hid_pages) {
	    descriptor_len += page->add_descriptor(&descriptor[descriptor_len]);
	    report_size += page->get_report_size();
	}

	descriptor[descriptor_len++] = 0xc0;                    // END_COLLECTION
	descriptor[descriptor_len++] = 0xc0;                    // END_COLLECTION

	HID::initialize(name, descriptor, descriptor_len, subclass);

	report = (uint8_t *) fatal_malloc(sizeof(*report) * report_size);
	report[0] = 0xa1;
    }

    void can_send_now() override {
	int pos = 1;
	for (auto page : hid_pages) {
	    page->fill_report(&report[pos]);
	    pos += page->get_report_size();
	}
	send_report(report, report_size);
    }

private:
    int report_size;
    uint8_t *report;

    static const int subclass = 0x580;
    static const int max_descriptor_len = 1024;
    uint8_t descriptor[max_descriptor_len];
    std::list<HIDPage *> hid_pages;
};

#endif
