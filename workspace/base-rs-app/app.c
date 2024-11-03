/**
 * Hello EV3
 *
 * This is a program used to test the whole platform.
 */

#include <ctype.h>
#include "ev3api.h"
#include "app.h"

void main_task(intptr_t unused) {
	ledcolor_t colors[4] = {LED_RED, LED_ORANGE, LED_GREEN, LED_OFF};
	int color = 0;

	for(;;) {
		ev3_led_set_color(colors[color]);

		ev3_schedule_bluetooth_agent_task();
		ev3_bt_write_value(0x12345678);

		color += 1;
		color %= 4;
		tslp_tsk(500U);
	}
}
