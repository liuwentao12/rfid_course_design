#ifndef DOOR_HARDWARE_H
#define DOOR_HARDWARE_H

#ifdef __cplusplus
extern "C" {
#endif

void DoorHardware_Init(void);
void DoorHardware_OnAccessGranted(void);
void DoorHardware_OnAccessDenied(void);
void DoorHardware_OnAlert(void);

#ifdef __cplusplus
}
#endif

#endif
