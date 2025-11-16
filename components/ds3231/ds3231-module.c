#include "include/ds3231-module.h"
#include "sdkconfig.h"
#include "driver/i2c_master.h"
#include "driver/i2c_slave.h"
#include "freertos/FreeRTOS.h"

#define I2C_MASTER_SCL_IO        CONFIG_I2C_MASTER_SCL
#define I2C_MASTER_SDA_IO        CONFIG_I2C_MASTER_SDA
#define DS3231_SENSOR_ADDR       0x68
#define I2C_MASTER_NUM           I2C_NUM_0
#define I2C_MASTER_FREQ_HZ       CONFIG_I2C_MASTER_FREQUENCY
#define DS3231_REG_TIME          0x00


static inline uint8_t dec_to_bcd(uint8_t val) {
 return ((val / 10) << 4) | (val % 10);
}

void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle) {

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
  .scl_speed_hz = CONFIG_I2C_MASTER_FREQUENCY,
};

 ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_cfg, dev_handle));
}

void register_read(i2c_master_dev_handle_t *dev_handle, uint8_t reg_addr, uint8_t *data, size_t len) {
 ESP_ERROR_CHECK(i2c_master_transmit_receive(*dev_handle, &reg_addr, 1, data, len, 1000));
}

void set_init_time(i2c_master_dev_handle_t *dev_handle, Time *t) {

 uint8_t buf[1 + 7];

 buf[0] = 0x00;

 buf[1] = dec_to_bcd(t->seconds) & 0x7F;
 buf[2] = dec_to_bcd(t->minutes) & 0x7F;
 buf[3] = dec_to_bcd(t->hours) & 0x3F;
 /*
  *I have set only dummy values for the day of the week, year, month and date, because the project doesn't need those
  *values to be there for logging, because I would need to set up the time again and again whenever I am running the tool
  *which is why it doesn't make sense to log the day as well.
  */
 buf[4] = 1;           // day of week (1–7) → just set 1
 buf[5] = dec_to_bcd(1);  // date → fake 1
 buf[6] = dec_to_bcd(1);  // month → fake Jan
 buf[7] = dec_to_bcd(24); // year (20YY → 2024)

 ESP_ERROR_CHECK(i2c_master_transmit(*dev_handle, buf, sizeof(buf), 1000));

}


