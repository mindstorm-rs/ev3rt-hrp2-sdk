#pragma once

extern bool_t ev3_spp_master_test_is_connected();
extern ER   spp_master_test_connect_ev3(const uint8_t *addr, const char *pin);
extern FILE*  ev3_spp_master_test_open_file();
