#include "pi.h"
#include "bluetooth/bluetooth.h"
#include "deep-sleep.h"
#include "gamepad.h"
#include "pico-adc.h"
#include "pico-joystick.h"

#define N_REGIONS 9

#define CENTER 0
#define UP    (1 << 0)
#define DOWN  (1 << 1)
#define LEFT  (1 << 2)
#define RIGHT (1 << 3)
#define SAME  (1 << 4)

#define UL (UP | LEFT)
#define UR (UP | RIGHT)
#define LT (LEFT)
#define RT (RIGHT)
#define DL (DOWN | LEFT)
#define DW (DOWN)
#define DR (DOWN | RIGHT)
#define SM (SAME)
#define NO (CENTER)

static uint8_t map_8_way[N_REGIONS * N_REGIONS] = {
    UL, UL, UL, UP, UP, UP, UR, UR, UR,
    UL, UL, UL, UP, UP, UP, UR, UR, UR,
    UL, UL, UL, UP, UP, UP, UR, UR, UR,
    LT, LT, LT, NO, NO, NO, RT, RT, RT,
    LT, LT, LT, NO, NO, NO, RT, RT, RT,
    LT, LT, LT, NO, NO, NO, RT, RT, RT,
    DL, DL, DL, DW, DW, DW, DR, DR, DR,
    DL, DL, DL, DW, DW, DW, DR, DR, DR,
    DL, DL, DL, DW, DW, DW, DR, DR, DR,
};

static uint8_t map_4_way[N_REGIONS * N_REGIONS] = {
    SM, UP, UP, UP, UP, UP, UP, UP, SM,
    LT, SM, UP, UP, UP, UP, UP, SM, RT,
    LT, LT, SM, UP, UP, UP, SM, RT, RT,
    LT, LT, LT, NO, NO, NO, RT, RT, RT,
    LT, LT, LT, NO, NO, NO, RT, RT, RT,
    LT, LT, LT, NO, NO, NO, RT, RT, RT,
    LT, LT, SM, DW, DW, DW, SM, RT, RT,
    LT, SM, DW, DW, DW, DW, DW, SM, RT,
    SM, DW, DW, DW, DW, DW, DW, DW, SM,
};

static uint8_t map_qbert[N_REGIONS * N_REGIONS] = {
    LT, LT, LT, LT, SM, UP, UP, UP, UP,
    LT, LT, LT, LT, SM, UP, UP, UP, UP,
    LT, LT, LT, LT, NO, UP, UP, UP, UP,
    LT, LT, LT, NO, NO, NO, UP, UP, UP,
    SM, SM, NO, NO, NO, NO, NO, SM, SM,
    DW, DW, DW, NO, NO, NO, RT, RT, RT,
    DW, DW, DW, DW, NO, RT, RT, RT, RT,
    DW, DW, DW, DW, SM, RT, RT, RT, RT,
    DW, DW, DW, DW, SM, RT, RT, RT, RT,
};

static uint8_t map_prefer_diagonals[N_REGIONS * N_REGIONS] = {
    UL, UL, SM, UP, UP, UP, SM, UR, UR,
    UL, UL, UL, UP, UP, UP, UR, UR, UR,
    SM, UL, UL, SM, UP, SM, UR, UR, SM,
    LT, LT, SM, UL, NO, UR, SM, RT, RT,
    LT, LT, LT, NO, NO, NO, RT, RT, RT,
    LT, LT, SM, DL, NO, DR, SM, RT, RT,
    SM, DL, DL, SM, DW, SM, DR, DR, SM,
    DL, DL, DL, DW, DW, DW, DR, DR, DR,
    DL, DL, SM, DW, DW, DW, SM, DR, DR,
};

class Sleeper : public DeepSleeper {
public:
    Sleeper(int gpio) : DeepSleeper(gpio, 10*60*1000) {
    }

    void pre_sleep(void) override {
	pico_joystick_go_to_sleep();
    }
};

class Joystick : public Gamepad {
public:
    Joystick(DeepSleeper *sleeper) : sleeper(sleeper) {
    }

    void can_send_now() override {
	sleeper->prod();
	Gamepad::can_send_now();
    }

    void on_connect() override {
	sleeper->prod();
	Gamepad::on_connect();
	connected = true;
    }

    void on_disconnect() override {
	Gamepad::on_disconnect();
	connected = false;
    }

    void wait_connected() {
	while (! connected) ms_sleep(1000);
    }

private:
    DeepSleeper *sleeper;
    bool connected = false;
};

static struct {
    int gpio;
    const char *name;
    GPInput *input;
} buttons[] = {
    {-1, "up" },	/* #1 */
    {-1, "down" },
    {-1, "left" },
    {-1, "right" },
    {13, "b1" },	/* #5 */
    { 9, "b2" },
    {-1, "b3" },
    { 5, "start" },	/* #8 */
    { 2, "select" },
    {-1, "meta" },
    {26, "thumb-switch" },
    {21, "program-mode" },
};

static const int n_buttons = sizeof(buttons) / sizeof(*buttons);

static GPInput *get_button(const char *name) {
    for (int i = 0; i < n_buttons; i++) {
	if (strcmp(buttons[i].name, name) == 0) return buttons[i].input;
    }
    return NULL;
}

static void dump_map(uint8_t *map) {
    for (int y = 0; y < N_REGIONS; y++) {
	if (y == 3 || y == 6) {
	    for (int i = 0; i < N_REGIONS + 3; i++) printf("-");
	    printf("\n");
	}
	for (int x = 0; x < N_REGIONS; x++) {
	    uint8_t action = map[x + y*N_REGIONS];
	    if (x == 3 || x == 6) printf("|");
	    if (action == SAME) printf("|same");
	    else printf("|%s%s%s%s", (action & LEFT) ? "L" : " ", (action & RIGHT) ? "R" : " ", (action & UP) ? "U" : " ", (action & DOWN) ? "D" : " ");
	}
	printf("|\n");
    }
}

static void threads_main(int argc, char **argv) {
    for (int i = 0; i < n_buttons; i++) {
	if (buttons[i].gpio >= 0) {
	    printf("Initializing %s on %d\n", buttons[i].name, buttons[i].gpio);
	    buttons[i].input = new GPInput(buttons[i].gpio);
	    buttons[i].input->set_pullup_up();
	}
    }

    ADC *adc = new PicoADC();

    GPInput *start  = get_button("start");
    GPInput *select = get_button("select");
    GPInput *b1     = get_button("b1");
    GPInput *b2     = get_button("b2");
    GPInput *program_mode = get_button("program-mode");

    if (start == NULL) printf("FAILED TO GET START\n");
    if (select == NULL) printf("FAILED TO GET SELECT\n");
    if (b1 == NULL) printf("FAILED TO GET B1\n");
    if (b2 == NULL) printf("FAILED TO GET B2\n");
    if (program_mode == NULL) printf("FAILED TO GET PROGRAM-MODE\n");

    pico_joystick_boot(b1, 13, start, "joystick");

    Joystick *joystick = new Joystick(new Sleeper(2));
    HIDButtons *hid_buttons = new HIDButtons(joystick, 1, n_buttons+1);
    joystick->add_hid_page(hid_buttons);
    joystick->initialize("Pico Thumbstick");
    bluetooth_start_gamepad("Pico Thumbstick");

    uint8_t *map = map_8_way;

    printf("Initial map:\n");
    dump_map(map);

    while (1) {
	joystick->wait_connected();

	if (program_mode->get()) {
	    uint8_t *new_map = map;

	    if (b1->get()) new_map = map_8_way;
	    else if (b2->get()) new_map = map_4_way;
	    else if (start->get()) new_map = map_qbert;
	    else if (select->get()) new_map = map_prefer_diagonals;

	    if (new_map != map) {
		map = new_map;
		printf("Loaded map:\n");
	        dump_map(map);
	    }

	    continue;
	}

	hid_buttons->begin_transaction();

	double x = adc->read_percentage(2);
	double y = adc->read_percentage(1);

	int x_region = (1 - x) * N_REGIONS;
	int y_region = (1 - y) * N_REGIONS;

	uint8_t action = map[x_region + y_region * N_REGIONS];

	if (action != SAME) {
	    hid_buttons->set_button(1, (action & UP) != 0);
	    hid_buttons->set_button(2, (action & DOWN) != 0);
	    hid_buttons->set_button(3, (action & LEFT) != 0);
	    hid_buttons->set_button(4, (action & RIGHT) != 0);
	}

	for (int i = 0; i < n_buttons; i++) {
	    if (buttons[i].gpio < 0) continue;
	    hid_buttons->set_button(i+1, buttons[i].input->get());
	}

	hid_buttons->end_transaction();
    }
}

int main(int argc, char **argv) {
   pi_init_with_threads(threads_main, argc, argv);
}
