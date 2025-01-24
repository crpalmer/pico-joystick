#ifndef __GAMEPAD_H__
#define __GAMEPAD_H__

#include "pi.h"
#include "mem.h"
#include "writer.h"
#include "bluetooth/hid.h"
#include "memory.h"
#include "pi-threads.h"
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

	state_bytes = n_buttons / 8;
	state = (uint8_t *) fatal_malloc(state_bytes);
	memset(state, 0, state_bytes);

	button_range[0] = first_button_id;
	button_range[1] = last_button_id;

	transaction_state = (uint8_t *) fatal_malloc(state_bytes);
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
	return state_bytes;
    }

    void fill_report(uint8_t *buf) override {
	memcpy(buf, state, state_bytes);
    }

    void set_button(int id, bool value) {
	//assert(id >= button_range[0] && id <= button_range[1]);
	id -= button_range[0];

	int byte = id/8;
	int bit = id%8;

	uint8_t *this_state;
	if (n_transactions) this_state = transaction_state;
	else this_state = state;

	uint8_t old_state = this_state[byte];
	if (value) this_state[byte] |= (1 << bit);
	else this_state[byte] &= ~(1 << bit);

	if (! n_transactions && old_state != this_state[byte]) {
	    hid->request_can_send_now();
	}
    }

    void begin_transaction() {
	if (n_transactions++ == 0) {
	    memcpy(transaction_state, state, state_bytes);
	}
    }

    void end_transaction() {
	if (--n_transactions == 0 && memcmp(state, transaction_state, state_bytes) != 0) {
	    memcpy(state, transaction_state, state_bytes);
	    hid->request_can_send_now();
	}
    }

private:
    HID *hid;
    int state_bytes;
    uint8_t *state;
    int n_buttons;
    int button_range[2];

    int n_transactions = 0;
    uint8_t *transaction_state;
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

class HIDSpinner : public HIDPage {
public:
    HIDSpinner(HID *hid) : hid(hid) {
	lock = new PiMutex();
    }


   int add_descriptor(uint8_t *descriptor) override {
	int i = 0;
	descriptor[i++] = 0x05;
	descriptor[i++] = 0x01;		// USAGE_PAGE (Generic Desktop Controls)
	descriptor[i++] = 0x09;
	descriptor[i++] = 0x30; 	// USAGE (X)
	descriptor[i++] = 0x09;
	descriptor[i++] = 0x31; 	// USAGE (X)
	descriptor[i++] = 0x16;
	descriptor[i++] = -1000 & 0xff; // LOGICAL_MINIMUM
	descriptor[i++] = -1000 >> 8;   // LOGICAL_MINIMUM
	descriptor[i++] = 0x26;
	descriptor[i++] = 1000 & 0xff;  // LOGICAL_MAXIMUM
	descriptor[i++] = 1000 >> 8;    // LOGICAL_MAXIMUM
	descriptor[i++] = 0x95;
	descriptor[i++] = 2;            // REPORT_COUNT
	descriptor[i++] = 0x75;
	descriptor[i++] = 16;           // REPORT_SIZE
	descriptor[i++] = 0x81;
	descriptor[i++] = 0x06;         // INPUT (Data,Var,Rel)

	return i;
    }

    int get_report_size() override {
	return 4;
    }

    void fill_report(uint8_t *buf) override {
	lock->lock();
	int report_ticks = ticks();
	delta = 0;
	lock->unlock();

	buf[0] = report_ticks & 0xff;
	buf[1] = report_ticks >> 8;
	buf[2] = buf[3] = 0;
    }

    void set_position(double position) {
	lock->lock();

	double delta = position - this->position;

	if (delta > 0.5) delta = position - (1 + this->position);
	if (delta < -0.5) delta = (1 + position) - this->position;

	this->delta += delta;
	this->position = position;

	int report_ticks = ticks();

	lock->unlock();

	if (report_ticks != 0) hid->request_can_send_now();
    }

private:
    HID *hid;
    PiMutex *lock;
    double position = 0;
    double delta = 0;

    int ticks() { return delta * 1000; }
};

class HIDController : public HID {
public:
    HIDController(uint8_t usage) : usage(usage) {
    }

    void add_hid_page(HIDPage *page) {
	hid_pages.push_back(page);
    }

    void initialize(const char *name) {
	int descriptor_len = 0;
	descriptor[descriptor_len++] = 0x05;
	descriptor[descriptor_len++] = 0x01; // USAGE_PAGE (Generic Desktop)
	descriptor[descriptor_len++] = 0x09;
	descriptor[descriptor_len++] = usage; // USAGE (___)
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

    uint8_t usage;
    static const int subclass = 0x580;
    static const int max_descriptor_len = 1024;
    uint8_t descriptor[max_descriptor_len];
    std::list<HIDPage *> hid_pages;
};

class Gamepad : public HIDController {
public:
    Gamepad() : HIDController(0x04) {
    }
};

class Mouse : public HIDController {
public:
    Mouse() : HIDController(0x02) {
    }
};

#endif
