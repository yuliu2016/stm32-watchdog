#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#include "stm32f4xx_hal.h"

static HAL_StatusTypeDef
Watchdog_SetExpiration(uint32_t expirationMs)
{
  return HAL_OK;
}

static HAL_StatusTypeDef
Watchdog_Init(uint32_t expirationMs)
{
  return Watchdog_SetExpiration(expirationMs);
}


static void Watchdog_Refresh()
{
  // IWDG->KR = 0x0000AAAAu;
}

#endif