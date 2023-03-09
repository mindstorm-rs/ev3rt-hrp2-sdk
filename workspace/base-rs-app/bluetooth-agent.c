#include "ev3api.h"
#include "app.h"
#include "syssvc/serial.h"
#include <unistd.h>
#include <ctype.h>
#include <string.h>

static char write_buffer[BLUETOOTH_BUF_SIZE];
static size_t write_buffer_length = 0;

static uint32_t bluetooth_task_period_ms = 50;

void bluetooth_agent_set_period_ms(uint32_t ms) {
    bluetooth_task_period_ms = ms;
}

static uint32_t bluetooth_agent_write_done = 0;
static uint32_t bluetooth_agent_write_todo = 0;

void bluetooth_agent_schedule_write(uint8_t* buf, size_t size) {
    if (size > BLUETOOTH_BUF_SIZE) {
        size = BLUETOOTH_BUF_SIZE;
    }

    uint32_t last_done = bluetooth_agent_write_done >> 16;
    uint32_t todo = (last_done + 1) % 0xFFFF;

    memcpy(write_buffer, buf, size);
    write_buffer_length = size;
    bluetooth_agent_write_todo = (todo << 16) | size;
}

static bool_t bluetooth_agent_received_pending = false;
static char bluetooth_agent_received_char = '\0';

char bluetooth_agent_get_last_char() {
    if (bluetooth_agent_received_pending) {
        bluetooth_agent_received_pending = false;
        return bluetooth_agent_received_char;
    } else {
        return 0;
    }
}

/*
 * TODO: Access to bluetooth_agent_write_todo and write_buffer swhould be synchronized
 */
static bool_t bluetooth_agent_task_active = false;
void bluetooth_agent_task(intptr_t unused) {
    ev3_serial_open_file(EV3_SERIAL_BT);
    bluetooth_agent_task_active = true;
 
    while (1) {
        if (bluetooth_agent_write_todo != bluetooth_agent_write_done) {
            serial_wri_dat(SIO_PORT_BT, &write_buffer[0], (uint_t) (bluetooth_agent_write_todo & 0xffff));
            bluetooth_agent_write_done = bluetooth_agent_write_todo;
        }

        bluetooth_agent_received_pending = serial_get_char(EV3_SERIAL_BT, &bluetooth_agent_received_char);

        tslp_tsk(bluetooth_task_period_ms);
    }
}

bool_t schedule_bluetooth_agent_task() {
    bool_t connected = ev3_bluetooth_is_connected();
    if (connected) {
        if (!bluetooth_agent_task_active) {
            act_tsk(BT_AGENT_TASK);
        }
    } else {
        if (bluetooth_agent_task_active) {
            ter_tsk(BT_AGENT_TASK);
        }
    }
    return connected;
}
