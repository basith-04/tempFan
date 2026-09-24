#include "tmp117.h"

extern I2C_HandleTypeDef hi2c2;

/* This STM32 HAL expects the 7-bit address in bits 7:1. */
#define TMP117_HAL_ADDRESS (TMP117_I2C_ADDRESS_7BIT << 1)
#define TMP117_I2C_TIMEOUT_MS 25U

HAL_StatusTypeDef TMP117_IsReady(void)
{
  return HAL_I2C_IsDeviceReady(&hi2c2, TMP117_HAL_ADDRESS, 2U,
                               TMP117_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef TMP117_ReadRegister(uint8_t reg, uint16_t *value)
{
  uint8_t bytes[2];
  HAL_StatusTypeDef status;

  if (value == NULL)
  {
    return HAL_ERROR;
  }
  status = HAL_I2C_Mem_Read(&hi2c2, TMP117_HAL_ADDRESS, reg,
                            I2C_MEMADD_SIZE_8BIT, bytes, 2U,
                            TMP117_I2C_TIMEOUT_MS);
  if (status == HAL_OK)
  {
    *value = ((uint16_t)bytes[0] << 8) | bytes[1];
  }
  return status;
}

HAL_StatusTypeDef TMP117_WriteRegister(uint8_t reg, uint16_t value)
{
  uint8_t bytes[2] = {(uint8_t)(value >> 8), (uint8_t)value};
  return HAL_I2C_Mem_Write(&hi2c2, TMP117_HAL_ADDRESS, reg,
                           I2C_MEMADD_SIZE_8BIT, bytes, 2U,
                           TMP117_I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef TMP117_ReadRaw(int16_t *raw)
{
  uint16_t value;
  HAL_StatusTypeDef status;

  if (raw == NULL)
  {
    return HAL_ERROR;
  }
  status = TMP117_ReadRegister(TMP117_REG_TEMPERATURE, &value);
  if (status != HAL_OK)
  {
    return status;
  }
  /* 0x8000 is the reset result (-256 C), before the first conversion. */
  if (value == 0x8000U)
  {
    return HAL_BUSY;
  }
  *raw = (int16_t)value;
  return HAL_OK;
}

HAL_StatusTypeDef TMP117_ReadTemperatureC(float *temperature)
{
  int16_t raw;
  HAL_StatusTypeDef status;

  if (temperature == NULL)
  {
    return HAL_ERROR;
  }
  status = TMP117_ReadRaw(&raw);
  if (status == HAL_OK)
  {
    *temperature = (float)raw * 0.0078125f;
  }
  return status;
}

HAL_StatusTypeDef TMP117_ReadDeviceID(uint16_t *device_id)
{
  return TMP117_ReadRegister(TMP117_REG_DEVICE_ID, device_id);
}

HAL_StatusTypeDef TMP117_Init(void)
{
  uint16_t device_id;
  HAL_StatusTypeDef status = TMP117_IsReady();
  if (status != HAL_OK)
  {
    return status;
  }
  status = TMP117_ReadDeviceID(&device_id);
  if (status != HAL_OK)
  {
    return status;
  }
  /* The upper nibble is a device revision field. */
  return ((device_id & 0x0FFFU) == 0x0117U) ? HAL_OK : HAL_ERROR;
}
