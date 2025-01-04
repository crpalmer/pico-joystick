#include "pico-joystick.h"

#define WAKE_GPIO 28

static Button *configure_wakeup_game_button(Button *button) {
    button->set_pullup_down();
    button->set_is_inverted();
    return button;
}

static Button *configure_ui_button(Button *button) {
    button->set_pullup_up();
    button->set_debounce(10);
    return button;
}

static Button *configure_game_button(Button *button) {
    button->set_pullup_up();
    return button;
}

static void threads_main(int argc, char **argv) {
    Button *select = configure_ui_button(new Button(17, "select"));
    Button *b1     = configure_wakeup_game_button(new Button(28, "button-1"));

    pico_joystick_on_boot("pico-joystick", b1, WAKE_GPIO, new GPOutput(27), new GPOutput(21), select);

    Button *down  = configure_game_button(new Button(2, "down"));
    Button *up    = configure_game_button(new Button(3, "up"));
    Button *right = configure_game_button(new Button(4, "right"));
    Button *left  = configure_game_button(new Button(5, "left"));
    Button *meta  = configure_ui_button  (new Button(6, "meta"));
    Button *b2    = configure_game_button(new Button(9, "button-2"));
    Button *start = configure_ui_button(new Button(14, "start"));
    //Button *b3    = configure_game_button(new Button(22, "button-3"));

    down->set_button_id(31);
    up->set_button_id(30);
    right->set_button_id(29);
    left->set_button_id(28);
    meta->set_button_id(27);
    start->set_button_id(26);
    select->set_button_id(25);
    b2->set_button_id(24);
    b1->set_button_id(23);
    //b3->set_button_id(22);

    pico_joystick_start("Pico Joystick");
}
    
int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}

