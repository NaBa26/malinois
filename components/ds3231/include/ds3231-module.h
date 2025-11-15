#pragma once
#include <stddef.h>
#include <driver/i2c_types.h>


void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle);
void register_read(i2c_master_dev_handle_t *dev_handle, uint8_t reg_addr, uint8_t *data, size_t len);
void set_init_time(i2c_master_dev_handle_t *dev_handle, uint8_t *write_buffer, size_t write_size, int xfer_timeout_ms);