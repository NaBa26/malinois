#include "include/ds3231-module.h"
#include "sdkconfig.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"

// ----------------------------
//   DS3231 + I2C Definitions
// ----------------------------

#define I2C_MASTER_SCL_IO        CONFIG_I2C_MASTER_SCL
#define I2C_MASTER_SDA_IO        CONFIG_I2C_MASTER_SDA
#define DS3231_SENSOR_ADDR       0x68
#define I2C_MASTER_NUM           I2C_NUM_0
#define I2C_MASTER_FREQ_HZ       CONFIG_I2C_MASTER_FREQUENCY
#define DS3231_REG_TIME          0x00



static inline uint8_t dec_to_bcd(uint8_t val) {
    return ((val / 10) << 4) | (val % 10);
}


// ---------------------------------------------------------
// Initialize I²C master bus and register DS3231 as a device
// ---------------------------------------------------------
void i2c_master_init(i2c_master_bus_handle_t *bus_handle,
                     i2c_master_dev_handle_t *dev_handle)
{
    const i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, bus_handle));

    const i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = DS3231_SENSOR_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_cfg, dev_handle));
}


// ---------------------------------------------------------
// Read arbitrary register(s) from DS3231
// reg_addr: starting register
// data: buffer where data will be stored
// len: number of bytes to read
// ---------------------------------------------------------
void register_read(i2c_master_dev_handle_t *dev_handle,
                   uint8_t reg_addr,
                   uint8_t *data,
                   size_t len)
{
    ESP_ERROR_CHECK(i2c_master_transmit_receive(
        *dev_handle,
        &reg_addr, 1,
        data, len,
        1000
    ));
}


// ---------------------------------------------------------
// Write initial time into DS3231 registers
//
// DS3231 time register layout:
//  0x00 - Seconds (BCD)
//  0x01 - Minutes (BCD)
//  0x02 - Hours   (BCD)
//  0x03 - Day of week (1–7)
//  0x04 - Date
//  0x05 - Month
//  0x06 - Year (00–99)
//
// In this project: Only H/M/S matter. Rest are dummy values.
// ---------------------------------------------------------
void set_init_time(i2c_master_dev_handle_t *dev_handle, Time *t)
{
    uint8_t buf[1 + 7];

    buf[0] = DS3231_REG_TIME;

    buf[1] = dec_to_bcd(t->seconds) & 0x7F;
    buf[2] = dec_to_bcd(t->minutes) & 0x7F;
    buf[3] = dec_to_bcd(t->hours)   & 0x3F;

    // These values don't matter for my logs:
    buf[4] = 1;             // Day of week (dummy)
    buf[5] = dec_to_bcd(1); // Date
    buf[6] = dec_to_bcd(1); // Month
    buf[7] = dec_to_bcd(24); // Year → "2024" (dummy)


    ESP_ERROR_CHECK(i2c_master_transmit(*dev_handle, buf, sizeof(buf), 1000));
}
