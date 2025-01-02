#ifndef __PICO_JOYSTICK_H__
#define __PICO_JOYSTICK_H__

#include "gp-input.h"
#include "io.h"
#include "pi-threads.h"

class GamePad;

class Button : public GPInput, public InputNotifier, PiThread {
public:
    Button(int button_id, int gpio, const char *name = "button");

    void main(void) override;
    void on_change(void) override;

private:
    int last_value = -1;
    int button_id;
};

class GamePad *pico_joystick_on_boot(const char *hostname, int bootloader_gpio, int wake_gpio, int power_led_gpio);
void pico_joystick_start(const char *bluetooth_name);

#endif
