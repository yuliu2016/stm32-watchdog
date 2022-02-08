#include "watchdog.h"

/**
 * @author Austin
 * 
 * Helper functions for configuring and refreshing the watchdog
 */


// Reference:
// RM0316 - STM32F303 Reference Manual, Revision 8
// Section 25: Independent Watchdog


// The watchdog operates on the independent LSI clock (internal 
// low speed oscillator) at 32kHz


// Configuration Procedure (Section 25.3.2):
// 1. Enable the IWDG by writing 0x0000 CCCC in the IWDG_KR register.
// 2. Enable register access by writing 0x0000 5555 in the IWDG_KR register.
// 3. Write the IWDG prescaler by programming IWDG_PR from 0 to 7.
// 4. Write the reload register (IWDG_RLR).
// 5. Wait for the registers to be updated (IWDG_SR = 0x0000 0000).
// 6. Refresh the counter value with IWDG_RLR (IWDG_KR = 0x0000 AAAA)



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


/**
 * @brief Initialize the onboard independent watchdog.
 * The watchdog is started immediately on success, and
 * must be refreshed within the expiration time.
 * 
 * @param expirationMs time until the watchdog expires
 * @return HAL_OK if configured correctly
 */
HAL_StatusTypeDef
Watchdog_Init(uint32_t expirationMs)
{

  // Check upper bound for expiration time
  // (before enabling the watchdog)
  if (expirationMs >= MAX_EXPIRATION) {
    return HAL_ERROR;
  }

  // Set flag to disable the watchdog during CPU halt
  // (Section 33.16.4 - APB1_FZ register)
  __HAL_DBGMCU_FREEZE_IWDG();

  // Enable the watchdog. Turns on the LSI 32kHz clock
  IWDG->KR = 0x0000CCCCu;

  // Set the expiration time
  return Watchdog_SetExpiration(expirationMs);
}



/**
 * @brief Set the expiration time of the watchdog. 
 * The watchdog is reset immediately on success, and
 * must be refreshed within the new expiration time.
 * 
 * @param expirationMs time until the watchdog expires
 * @return HAL_OK if configured correctly
 */
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

  // Increase prescaler if expiration time is too high
  // to fit in the reload register.
  // Section 25.4.2 - prescaler values:
  // - 3: divider /32
  // - 4: divider /64
  // - 5: divider /128
  // - 6: divider /256
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
  // and re-enable register write protection
  Watchdog_Refresh();

  return HAL_OK;
}



/**
 * @brief Set the key register to 0xAAAA to reset the timer counter
 * to the value in the reload register and prevent a system reset.
 * Must be called within the expiration time.
 */
void 
Watchdog_Refresh()
{
  IWDG->KR = 0x0000AAAAu;
}
