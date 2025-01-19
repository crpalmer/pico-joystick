#ifndef __PICO_JOYSTICK_H__
#define __PICO_JOYSTICK_H__

#include "gp-input.h"
#include "gp-output.h"
#include "io.h"
#include "gamepad.h"
#include "pi-threads.h"

class Button : public GPInput, public InputNotifier, PiThread {
public:
    Button(int gpio, const char *name = "button");

    void set_button_id(Gamepad *gp, int button_id);

    void main(void) override;
    void on_change(void) override;

private:
    class Gamepad *gp;
    int last_value = -1;
    int button_id = -1;
};

void pico_joystick_go_to_sleep();

void pico_joystick_boot(Input *bootloader_button = NULL, int wakeup_gpio = -1, Input *wifi_button = NULL, const char *hostname = NULL);

#endif
