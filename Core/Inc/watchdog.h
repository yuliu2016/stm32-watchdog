#ifndef __WATCHDOG_H
#define __WATCHDOG_H

#include "stm32f4xx_hal.h"

HAL_StatusTypeDef Watchdog_Init(uint32_t expirationMs);

HAL_StatusTypeDef Watchdog_SetExpiration(uint32_t expirationMs);

void Watchdog_Refresh();

#endif