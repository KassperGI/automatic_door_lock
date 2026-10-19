/*
 * i2c-lcd.h
 *
 *  Created on: 19-Oct-2026
 *      Author: 91799
 */

#ifndef I2C_LCD_H_
#define I2C_LCD_H_

#include "stm32f1xx_hal.h"

void lcd_init(void);   // initialize lcd
void lcd_send_cmd(char cmd);  // send command to the lcd
void lcd_send_data(char data);  // send data to the lcd
void lcd_send_string(char *str);  // send string to the lcd
void lcd_put_cur(int row, int col);  // put cursor at the entered position row (0 or 1), col (0-15);
void lcd_clear(void);

#endif /* I2C_LCD_H_ */
