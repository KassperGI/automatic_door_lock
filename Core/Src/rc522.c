/*
 * rc522.c
 *
 *  Created on: 20-Oct-2026
 *      Author: 91799
 */
#include "rc522.h"
extern SPI_HandleTypeDef hspi1;

void MFRC522_WriteRegister(uint8_t addr, uint8_t val) {
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    uint8_t data[2] = {(addr << 1) & 0x7E, val};
    HAL_SPI_Transmit(&hspi1, data, 2, 100);
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

uint8_t MFRC522_ReadRegister(uint8_t addr) {
    uint8_t tx_data[2] = {((addr << 1) & 0x7E) | 0x80, 0x00};
    uint8_t rx_data[2] = {0, 0};
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, 2, 100);
    HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
    return rx_data[1];
}

void MFRC522_SetBitMask(uint8_t reg, uint8_t mask) {
    MFRC522_WriteRegister(reg, MFRC522_ReadRegister(reg) | mask);
}

void MFRC522_ClearBitMask(uint8_t reg, uint8_t mask) {
    MFRC522_WriteRegister(reg, MFRC522_ReadRegister(reg) & (~mask));
}

void MFRC522_Init(void) {
    HAL_GPIO_WritePin(RST_PORT, RST_PIN, GPIO_PIN_SET);
    MFRC522_WriteRegister(CommandReg, PCD_RESETPHASE);
    MFRC522_WriteRegister(TModeReg, 0x8D);
    MFRC522_WriteRegister(TPrescalerReg, 0x3E);
    MFRC522_WriteRegister(TReloadRegL, 30);
    MFRC522_WriteRegister(TReloadRegH, 0);
    MFRC522_WriteRegister(TxAutoReg, 0x40);
    MFRC522_WriteRegister(ModeReg, 0x3D);
    if (!(MFRC522_ReadRegister(TxControlReg) & 0x03)) MFRC522_SetBitMask(TxControlReg, 0x03);
}

uint8_t MFRC522_ToCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen) {
    uint8_t status = 2, irqEn = 0x00, waitIRq = 0x00, lastBits, n;
    uint16_t i;
    if (command == PCD_AUTHENT) { irqEn = 0x12; waitIRq = 0x10; }
    else if (command == PCD_TRANSCEIVE) { irqEn = 0x77; waitIRq = 0x30; }

    MFRC522_WriteRegister(ComIEnReg, irqEn | 0x80);
    MFRC522_ClearBitMask(ComIrqReg, 0x80);
    MFRC522_SetBitMask(FIFOLevelReg, 0x80);
    MFRC522_WriteRegister(CommandReg, PCD_IDLE);

    for (i = 0; i < sendLen; i++) MFRC522_WriteRegister(FIFODataReg, sendData[i]);
    MFRC522_WriteRegister(CommandReg, command);
    if (command == PCD_TRANSCEIVE) MFRC522_SetBitMask(BitFramingReg, 0x80);

    i = 2000;
    do { n = MFRC522_ReadRegister(ComIrqReg); i--; } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));
    MFRC522_ClearBitMask(BitFramingReg, 0x80);

    if (i != 0 && !(MFRC522_ReadRegister(ErrorReg) & 0x1B)) {
        status = 0;
        if (n & irqEn & 0x01) status = 1;
        if (command == PCD_TRANSCEIVE) {
            n = MFRC522_ReadRegister(FIFOLevelReg);
            lastBits = MFRC522_ReadRegister(ControlReg) & 0x07;
            if (lastBits) *backLen = (n - 1) * 8 + lastBits; else *backLen = n * 8;
            if (n == 0) n = 1; if (n > 16) n = 16;
            for (i = 0; i < n; i++) backData[i] = MFRC522_ReadRegister(FIFODataReg);
        }
    } else status = 2;
    return status;
}

uint8_t MFRC522_Check(uint8_t* id) {
    uint8_t status;
    uint16_t backBits;
    MFRC522_WriteRegister(BitFramingReg, 0x07);
    id[0] = 0x52; // PICC_REQALL
    status = MFRC522_ToCard(PCD_TRANSCEIVE, id, 1, id, &backBits);
    if (status == 0) {
        MFRC522_WriteRegister(BitFramingReg, 0x00);
        id[0] = 0x93; id[1] = 0x20;
        status = MFRC522_ToCard(PCD_TRANSCEIVE, id, 2, id, &backBits);
        if (status == 0) {
            uint8_t serNumCheck = 0;
            for (uint8_t i = 0; i < 4; i++) serNumCheck ^= id[i];
            if (serNumCheck != id[4]) status = 2;
        }
    }
    return status;
}

uint8_t MFRC522_Compare(uint8_t* CardID, uint8_t* CompareID) {
    for (uint8_t i = 0; i < 5; i++) {
        if (CardID[i] != CompareID[i]) return 0; // Fail
    }
    return 1; // Success
}
