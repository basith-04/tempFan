#include "fan.h"

extern TIM_HandleTypeDef htim2;

static uint8_t fan_percent;

void Fan_SetPercent(int16_t percent)
{
  uint32_t period_counts;
  uint32_t compare;

  if (percent < 0)
  {
    percent = 0;
  }
  else if (percent > 100)
  {
    percent = 100;
  }

  period_counts = __HAL_TIM_GET_AUTORELOAD(&htim2) + 1U;
  compare = (period_counts * (uint32_t)percent + 50U) / 100U;
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, compare);
  fan_percent = (uint8_t)percent;
}

HAL_StatusTypeDef Fan_Init(void)
{
  /* Set the compare register before enabling the timer output. */
  Fan_Stop();
  return HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

uint8_t Fan_GetPercent(void)
{
  return fan_percent;
}

void Fan_Stop(void)
{
  Fan_SetPercent(0);
}

void Fan_SetFullSpeed(void)
{
  Fan_SetPercent(100);
}
