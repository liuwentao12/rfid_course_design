#ifndef ACCESS_CONFIG_H
#define ACCESS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

#include "access_control.h"

/*
 * 把写在 access_config.c 中的固定授权卡加载到授权名单。
 * 返回成功加载的卡片数量。
 */
size_t AccessConfig_LoadAuthorizedCards(AccessControl *control);

#ifdef __cplusplus
}
#endif

#endif
