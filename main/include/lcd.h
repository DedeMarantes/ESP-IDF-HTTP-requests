#ifndef LCD_H
#define LCD_H

#include <stdio.h>
#include <stdint.h>
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"

#include <stdbool.h>

#define I2C_DEV_ADDR 0x27

void lcd_init(i2c_master_dev_handle_t i2c_dev_handle);
void lcd_send_string(i2c_master_dev_handle_t i2c_dev_handle, char *str);
void lcd_clear(i2c_master_dev_handle_t i2c_dev_handle);
void lcd_shift_left(i2c_master_dev_handle_t i2c_dev_handle);
void lcd_shift_right(i2c_master_dev_handle_t i2c_dev_handle);
void lcd_cursor_to_left(i2c_master_dev_handle_t i2c_dev_handle);
void lcd_cursor_to_right(i2c_master_dev_handle_t i2c_dev_handle);
void lcd_set_cursor(i2c_master_dev_handle_t i2c_dev_handle, uint8_t row, uint8_t col);
void lcd_cursor_blink(i2c_master_dev_handle_t i2c_dev_handle, bool blink);


#endif