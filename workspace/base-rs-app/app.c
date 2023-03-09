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
		char received = ev3_bluetooth_agent_get_last_char();

		char to_write = '0' + color;
		if (received != 0) {
			to_write = received;
		}
		ev3_bluetooth_agent_schedule_write((uint8_t*) &to_write, 1);

		color += 1;
		color %= 4;
		tslp_tsk(500U);
	}
}
