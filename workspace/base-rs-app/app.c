/**
 * Hello EV3
 *
 * This is a program used to test the whole platform.
 */

#include "ev3api.h"
#include "app.h"

void main_task(intptr_t unused) {
	ev3_led_set_color(LED_RED);
	tslp_tsk(1000U);
	ev3_led_set_color(LED_ORANGE);
	tslp_tsk(1000U);
	ev3_led_set_color(LED_GREEN);
	tslp_tsk(1000U);
	ev3_led_set_color(LED_OFF);
	tslp_tsk(1000U);
}
