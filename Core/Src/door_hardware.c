#include "door_hardware.h"

#include <stdbool.h>
#include <stdint.h>

#include "cmsis_os.h"
#include "main.h"

#ifndef DOOR_BUZZER_ACTIVE_STATE
#define DOOR_BUZZER_ACTIVE_STATE GPIO_PIN_SET
#endif

#ifndef DOOR_LOCK_ACTIVE_STATE
#define DOOR_LOCK_ACTIVE_STATE GPIO_PIN_RESET
#endif

#ifndef DOOR_UNLOCK_PULSE_MS
#define DOOR_UNLOCK_PULSE_MS 1500U
#endif

#if defined(DOOR_LOCK_Pin) && defined(DOOR_LOCK_GPIO_Port)
#define DOOR_HW_HAS_LOCK 1
#define DOOR_HW_LOCK_PIN DOOR_LOCK_Pin
#define DOOR_HW_LOCK_PORT DOOR_LOCK_GPIO_Port
#elif defined(LOCK_Pin) && defined(LOCK_GPIO_Port)
#define DOOR_HW_HAS_LOCK 1
#define DOOR_HW_LOCK_PIN LOCK_Pin
#define DOOR_HW_LOCK_PORT LOCK_GPIO_Port
#elif defined(LED_Pin) && defined(LED_GPIO_Port)
#define DOOR_HW_HAS_LOCK 1
#define DOOR_HW_LOCK_PIN LED_Pin
#define DOOR_HW_LOCK_PORT LED_GPIO_Port
#else
#define DOOR_HW_HAS_LOCK 0
#endif

static GPIO_PinState DoorHardware_Invert(GPIO_PinState state)
{
  return state == GPIO_PIN_SET ? GPIO_PIN_RESET : GPIO_PIN_SET;
}

static void DoorHardware_SetBuzzer(bool active)
{
#if defined(BEEF_Pin) && defined(BEEF_GPIO_Port)
  HAL_GPIO_WritePin(BEEF_GPIO_Port, BEEF_Pin,
                    active ? DOOR_BUZZER_ACTIVE_STATE : DoorHardware_Invert(DOOR_BUZZER_ACTIVE_STATE));
#else
  (void)active;
#endif
}

static void DoorHardware_SetLockUnlocked(bool unlocked)
{
#if DOOR_HW_HAS_LOCK
  HAL_GPIO_WritePin(DOOR_HW_LOCK_PORT, DOOR_HW_LOCK_PIN,
                    unlocked ? DOOR_LOCK_ACTIVE_STATE : DoorHardware_Invert(DOOR_LOCK_ACTIVE_STATE));
#else
  (void)unlocked;
#endif
}

static void DoorHardware_Beep(uint8_t count, uint32_t on_ms, uint32_t off_ms)
{
  for (uint8_t i = 0U; i < count; i++)
  {
    DoorHardware_SetBuzzer(true);
    osDelay(on_ms);
    DoorHardware_SetBuzzer(false);
    if ((uint8_t)(i + 1U) < count)
    {
      osDelay(off_ms);
    }
  }
}

void DoorHardware_Init(void)
{
  DoorHardware_SetBuzzer(false);
  DoorHardware_SetLockUnlocked(false);
}

void DoorHardware_OnAccessGranted(void)
{
  DoorHardware_SetLockUnlocked(true);
  DoorHardware_Beep(1U, 80U, 0U);
  osDelay(DOOR_UNLOCK_PULSE_MS);
  DoorHardware_SetLockUnlocked(false);
}

void DoorHardware_OnAccessDenied(void)
{
  DoorHardware_Beep(2U, 120U, 80U);
}

void DoorHardware_OnAlert(void)
{
  DoorHardware_Beep(5U, 70U, 50U);
}