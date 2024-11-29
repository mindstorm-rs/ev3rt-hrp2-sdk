#include "ev3api.h"
#include "platform_interface_layer.h"
#include "app.h"
#include "syssvc/serial.h"
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "spp_master_test_api.h"

/*
 * TODO: Access to bluetooth_agent_write_todo, buffers and buffer lenghts swhould be synchronized
 */
static FILE* bt = NULL;
static bool_t bluetooth_agent_task_active = false;

static uint32_t bluetooth_task_period_ms = 50;
static bool_t bluetooth_is_master = false;
void ev3_bluetooth_agent_set_period_ms(uint32_t ms) {
    bluetooth_task_period_ms = ms;
}
void ev3_bluetooth_agent_set_master(bool_t is_master) {
    bluetooth_is_master = is_master;
}

static uint32_t bluetooth_task_values_read = 0;
static uint32_t bluetooth_task_value_read = 0;
uint32_t *ev3_bluetooth_agent_values_read_ptr() {
    return &bluetooth_task_values_read;
}
uint32_t *ev3_bluetooth_agent_value_read_ptr() {
    return &bluetooth_task_value_read;
}

#define BT_DATA_BUFFER_SIZE 9

inline uint32_t decode_hex_digit(uint8_t c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else {
        return 0;
    }
}

inline uint8_t encode_hex_digit(uint32_t c) {
    if (c < 10) {
        return '0' + c;
    } else if (c < 16) {
        return 'A' + (c - 10);
    } else {
        return '0';
    }
}

static bool_t bt_read() {
    if (bt == NULL) {
        return false;
    }

    uint8_t buf[BT_DATA_BUFFER_SIZE];
    size_t read = fread(buf, 1, BT_DATA_BUFFER_SIZE, bt);
    if (read < BT_DATA_BUFFER_SIZE) {
        return false;
    }
    if (buf[BT_DATA_BUFFER_SIZE-1] != '\n') {
        return false;
    }

    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        uint8_t dh = buf[i * 2];
        uint8_t dl = buf[(i * 2) + 1];
        value <<= 4;
        value |= decode_hex_digit(dh);
        value <<= 4;
        value |= decode_hex_digit(dl);
    }

    bluetooth_task_values_read += 1;
    bluetooth_task_value_read = value;
    return true;
}

bool_t ev3_bt_write_value(uint32_t value) {
    if (bt == NULL) {
        return false;
    }

    char buf[BT_DATA_BUFFER_SIZE];
    for (int i = 0; i < 4; i++) {
        uint32_t dh = (value >> 28) & 0xf;
        uint32_t dl = (value >> 24) & 0xf;
        buf[i * 2] = encode_hex_digit(dh);
        buf[(i * 2) + 1] = encode_hex_digit(dl);
        value <<= 8;
    }
    buf[BT_DATA_BUFFER_SIZE-1] = '\n';

    size_t written = fwrite(buf, 1, BT_DATA_BUFFER_SIZE, bt);
    return written == BT_DATA_BUFFER_SIZE;
}

void bluetooth_agent_task(intptr_t unused) {
    if (bt == NULL) {
        syslog(LOG_ERROR, "Bluetooth file is closed");
        return;
    }
    bluetooth_agent_task_active = true;
 
    while (1) {
        if (bt_read()) {
            if (bluetooth_task_period_ms > 0) {
                tslp_tsk(bluetooth_task_period_ms);
            }
        }
    }
}

ER ev3_connect_to_bluetooth_device(const uint8_t *addr, const char *pin) {
    return spp_master_test_connect(addr, pin);
}

bool_t ev3_spp_master_test_is_connected() {
	T_SERIAL_RPOR rpor;
	ER ercd = serial_ref_por(SIO_PORT_SPP_MASTER_TEST, &rpor);
	return ercd == E_OK;
}

bool_t ev3_spp_slave_test_is_connected() {
	T_SERIAL_RPOR rpor;
	ER ercd = serial_ref_por(SIO_PORT_BT, &rpor);
	return ercd == E_OK;
}

bool_t ev3_check_bluetooth_is_connected() {
    if (bluetooth_is_master) {
        return ev3_spp_master_test_is_connected();
    } else {
        return ev3_spp_slave_test_is_connected();
    }
}

FILE* ev3_spp_master_test_open_file() {
	int fd = SIO_PORT_SPP_MASTER_TEST_FILENO;

    FILE *fp = fdopen(fd, "a+");
    if (fp != NULL)
    	setbuf(fp, NULL); /* IMPORTANT! */
    else assert(false); //API_ERROR("fdopen() failed, fd: %d.", fd);
    return fp;
}


bool_t ev3_schedule_bluetooth_agent_task() {
    bool_t connected = ev3_check_bluetooth_is_connected();
    if (connected) {
        if (bluetooth_is_master) {
            bt = ev3_spp_master_test_open_file();
        } else {
            bt = ev3_serial_open_file(EV3_SERIAL_BT);
        }
        if (bt == NULL) {
            syslog(LOG_ERROR, "Failed to open bluetooth file");
            return false;
        }
        if (!bluetooth_agent_task_active) {
            act_tsk(BT_AGENT_TASK);
        }
    } else {
        if (bluetooth_agent_task_active) {
            bluetooth_agent_task_active = false;
            ter_tsk(BT_AGENT_TASK);
        }
    }
    return connected;
}
