/*
 * rc522.h
 *
 *  Created on: 20-Oct-2026
 *      Author: 91799
 */

#ifndef RC522_H_
#define RC522_H_

#include "stm32f1xx_hal.h"

// Hardware Pin Definitions
#define CS_PORT GPIOA
#define CS_PIN GPIO_PIN_4
#define RST_PORT GPIOB
#define RST_PIN GPIO_PIN_0

// RC522 Firmware Commands & Registers
#define PCD_IDLE              0x00
#define PCD_AUTHENT           0x0E
#define PCD_TRANSCEIVE        0x0C
#define PCD_RESETPHASE        0x0F
#define CommandReg            0x01
#define ComIEnReg             0x02
#define ComIrqReg             0x04
#define ErrorReg              0x06
#define FIFODataReg           0x09
#define FIFOLevelReg          0x0A
#define ControlReg            0x0C
#define BitFramingReg         0x0D
#define ModeReg               0x11
#define TxControlReg          0x14
#define TxAutoReg             0x15
#define TModeReg              0x2A
#define TPrescalerReg         0x2B
#define TReloadRegH           0x2C
#define TReloadRegL           0x2D

// Functions
void MFRC522_Init(void);
uint8_t MFRC522_Check(uint8_t* id);
uint8_t MFRC522_Compare(uint8_t* CardID, uint8_t* CompareID);

#endif/* INC_RC522_H_ */
