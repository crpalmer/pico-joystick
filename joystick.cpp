#include "pico-joystick.h"

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
    Button *start  = configure_ui_button(new Button(6, "start"));
    Button *b1     = configure_wakeup_game_button(new Button(10, "button-1"));

    pico_joystick_on_boot("pico-joystick", b1, 10, new GPOutput(0), new GPOutput(1), start);

    Button *up     = configure_game_button(new Button(2, "up"));
    Button *down   = configure_game_button(new Button(3, "down"));
    Button *left   = configure_game_button(new Button(4, "left"));
    Button *right  = configure_game_button(new Button(5, "right"));
    Button *select = configure_ui_button  (new Button(7, "select"));
    Button *meta   = configure_ui_button  (new Button(8, "meta"));
    Button *b2     = configure_game_button(new Button(11, "button-2"));
    Button *b3     = configure_game_button(new Button(12, "button-3"));

    down->set_button_id(31);
    up->set_button_id(30);
    right->set_button_id(29);
    left->set_button_id(28);
    meta->set_button_id(27);
    start->set_button_id(26);
    select->set_button_id(25);
    b2->set_button_id(24);
    b1->set_button_id(23);
    b3->set_button_id(22);

    pico_joystick_start("Pico Joystick");
}
    
int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}

