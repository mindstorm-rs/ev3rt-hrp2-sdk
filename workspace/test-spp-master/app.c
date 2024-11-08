#include "ev3api.h"
#include "app.h"
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "spp_master_test_api.h"
#include "platform_interface_layer.h"
#include "syssvc/serial.h"

int32_t default_menu_font_width;
int32_t default_menu_font_height;

uint8_t remote_addr[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
const char* remote_pincode = "1234";

static FILE *bt_slave;

void bluetooth_task(intptr_t unused) {
	// Draw name
	int offset_x = 0, offset_y = MENU_FONT_HEIGHT * 2;
	ev3_lcd_draw_string("Remote Port 2: 0", offset_x, offset_y + MENU_FONT_HEIGHT * 0);

    static char buf[256];
    while (1) {
        memset(buf, 0, 256);
        size_t len = fread(buf, 1, 255, bt_slave);
        if (len > 0) {
            if (strncmp("SENSOR", buf, strlen("SENSOR")) == 0) { // Touch sensor value received
                int value;
                if (sscanf(buf, "SENSOR %d", &value) == 1) {
		            ev3_lcd_draw_string(value ? "1" : "0",
                            offset_x + strlen("Remote Port 2: ") * MENU_FONT_WIDTH, 
                            offset_y + MENU_FONT_HEIGHT * 0);
                } else assert(false);
            } else assert(false);
        } else dly_tsk(10);
    }
}

void main_task(intptr_t unused) {
    ev3_lcd_set_font(MENU_FONT);
    ev3_font_get_size(MENU_FONT, &default_menu_font_width, &default_menu_font_height);

	// Clear menu area
	ev3_lcd_fill_rect(0, 0, EV3_LCD_WIDTH, EV3_LCD_HEIGHT, EV3_LCD_WHITE);

	// Draw title
	const char *title = "BT SPP MASTER";
	int offset_x = 0, offset_y = 0;
	if (EV3_LCD_WIDTH - offset_x > strlen(title) * MENU_FONT_WIDTH)
		offset_x += (EV3_LCD_WIDTH - offset_x - strlen(title) * MENU_FONT_WIDTH) / 2;
	ev3_lcd_draw_string(title, offset_x, offset_y);

	// Draw name
	offset_x = 0;
	offset_y += MENU_FONT_HEIGHT;
	//ev3_lcd_draw_string("Connected: ", offset_x, offset_y + MENU_FONT_HEIGHT * 0);
	ev3_lcd_draw_string("Remote Port A: 0", offset_x, offset_y + MENU_FONT_HEIGHT * 2);

	// Draw status
    bt_slave = ev3_spp_master_test_open_file();
    act_tsk(BT_TASK);
	char lcdstr[100];
    bool_t connected = false;
    int power10 = 0;
	while (1) {
        // Connect to slave
        while (!ev3_spp_master_test_is_connected()) {
		    ev3_lcd_draw_string("N", offset_x + strlen("Connected: ") * MENU_FONT_WIDTH, offset_y + MENU_FONT_HEIGHT * 0);
            spp_master_test_connect_ev3(remote_addr, remote_pincode);
        }
		ev3_lcd_draw_string("Y", offset_x + strlen("Connected: ") * MENU_FONT_WIDTH, offset_y + MENU_FONT_HEIGHT * 0);

        // Control remote motor power
        power10++;
        if ((power10 % 100) == 0) {
		    sprintf(lcdstr, "%-3d", power10 / 10);
		    ev3_lcd_draw_string(lcdstr, offset_x + strlen("Remote Port A: ") * MENU_FONT_WIDTH,
                    offset_y + MENU_FONT_HEIGHT * 2);
            fprintf(bt_slave, "MOTOR %d\n", power10 / 10);
            if (power10 >= 1000) power10 = 0;
        } else fprintf(bt_slave, "SENSOR\n"); // or request sensor data

        dly_tsk(10);
	}
}

ER spp_master_test_connect_ev3(const uint8_t *addr, const char *pin) {
    return spp_master_test_connect(addr, pin); // TODO: check return value
}

bool_t ev3_spp_master_test_is_connected() {
	T_SERIAL_RPOR rpor;
	ER ercd = serial_ref_por(SIO_PORT_SPP_MASTER_TEST, &rpor);
	return ercd == E_OK;
}

FILE* ev3_spp_master_test_open_file() {
	int fd = SIO_PORT_SPP_MASTER_TEST_FILENO;

    FILE *fp = fdopen(fd, "a+");
    if (fp != NULL)
    	setbuf(fp, NULL); /* IMPORTANT! */
    else assert(false); //API_ERROR("fdopen() failed, fd: %d.", fd);
    return fp;
}

