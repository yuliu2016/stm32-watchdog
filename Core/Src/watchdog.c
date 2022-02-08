#include "watchdog.h"

/**
 * @author Austin
 * 
 * Helper functions for configuring and refreshing the watchdog
 */


// Reference:
// RM0316 - STM32F303 Reference Manual, Revision 8
// Section 25: Independent Watchdog



// Maximum allowed expiration time in ms
// = (max reload) * (max prescaler) * 1000 / (LSI clock speed)
// = 4096 * 256 * 1000 / 32000
// = 32768
#define MAX_EXPIRATION 32768


// Timeout to allow the status register to update in ms
// (max update time: 6 LSI clocks divided by the prescaler)
// = 6 * (max prescaler) * 1000 / (LSI clock speed)
// = 6 * 256 * 1000 / 32000
// = 48
#define STATUS_TIMEOUT 48


// Initialize the watchdog
// Watchdog is started immediately, so Watchdog_Refresh()
// must be called within expirationMs
HAL_StatusTypeDef
Watchdog_Init(uint32_t expirationMs)
{

  // Check upper bound for expiration time
  // (before enabling the watchdog)
  if (expirationMs >= MAX_EXPIRATION) {
    return HAL_ERROR;
  }

  // Set flag to disable the watchdog during CPU halt
  __HAL_DBGMCU_UNFREEZE_IWDG();

  // Enable the watchdog. Turns on the LSI 32kHz clock
  IWDG->KR = 0x0000CCCCu;

  // Set the expiration time
  return Watchdog_SetExpiration(expirationMs);
}



// Set the expiration time of the watchdog
// Watchdog is reset immediately on success
HAL_StatusTypeDef
Watchdog_SetExpiration(uint32_t expirationMs)
{

  // Check upper bound for expiration time
  if (expirationMs >= MAX_EXPIRATION) {
    return HAL_ERROR;
  }

  // 3 corresponds to a /32 division, which downclocks
  // the original 32kHz to 1kHz, or one count per ms
  uint32_t prescaler = 3;
  uint32_t reload = expirationMs;

  // Increase prescaler if expiration time is too big
  // to fit in the reload register
  while (reload > 4095) {
    prescaler += 1;
    reload >>= 1;
  }

  // Check whether the status register is reset. If not,
  // the protected registers cannot be updated
  if (IWDG->SR) {
    return HAL_ERROR;
  }

  // Enable write access of protected registers
  IWDG->KR = 0x00005555u;

  // Write to the prescaler and reload registers
  IWDG->PR = prescaler;
  IWDG->RLR = reload;

  // Wait for the status register to be updated
  // and return timeout if it takes too long
  uint32_t tickstart = HAL_GetTick();
  while (IWDG->SR) {
    if ((HAL_GetTick() - tickstart) > STATUS_TIMEOUT) {
      return HAL_TIMEOUT;
    }
  }

  // Refresh the watchdog with the new reload register
  // This re-enables register protection
  Watchdog_Refresh();

  return HAL_OK;
}



// Set the key register to 0xAAAA to reset the timer counter
// to the value in the reload register and prevent a reset.
// Must be called within the expiration time.
void 
Watchdog_Refresh()
{
  IWDG->KR = 0x0000AAAAu;
}
