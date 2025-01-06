#ifndef __PICO_JOYSTICK_H__
#define __PICO_JOYSTICK_H__

#include "gp-input.h"
#include "gp-output.h"
#include "io.h"
#include "pi-threads.h"

class GamePad;

class Button : public GPInput, public InputNotifier, PiThread {
public:
    Button(int gpio, const char *name = "button");

    void set_button_id(int button_id);

    void main(void) override;
    void on_change(void) override;

private:
    int last_value = -1;
    int button_id = -1;
};

extern void configure_button(GPInput *button);

class GamePad *pico_joystick_on_boot(GPInput *bootloader_button, int wakeup_gpio, GPOutput *power_led = NULL, GPOutput *bluetooth_led = NULL, GPInput *wakeup_button = NULL);
void pico_joystick_start(const char *bluetooth_name, const char *hostname);

#endif
