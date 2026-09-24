#ifndef TMP117_H
#define TMP117_H

#include "stm32f4xx_hal.h"

#define TMP117_I2C_ADDRESS_7BIT 0x48U
#define TMP117_REG_TEMPERATURE  0x00U
#define TMP117_REG_CONFIGURATION 0x01U
#define TMP117_REG_DEVICE_ID    0x0FU
#define TMP117_CONFIG_DATA_READY 0x2000U

HAL_StatusTypeDef TMP117_Init(void);
HAL_StatusTypeDef TMP117_IsReady(void);
HAL_StatusTypeDef TMP117_ReadRegister(uint8_t reg, uint16_t *value);
HAL_StatusTypeDef TMP117_WriteRegister(uint8_t reg, uint16_t value);
HAL_StatusTypeDef TMP117_ReadRaw(int16_t *raw);
HAL_StatusTypeDef TMP117_ReadTemperatureC(float *temperature);
HAL_StatusTypeDef TMP117_ReadDeviceID(uint16_t *device_id);

#endif
