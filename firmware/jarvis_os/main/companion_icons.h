#pragma once
#include "launcher_icons.h"
#define COMPANION_ICON_COUNT 7
enum {
    COMPANION_ICON_GARDEN = LAUNCHER_ICON_COUNT,
    COMPANION_ICON_FOCUS,
    COMPANION_ICON_CAPSULES,
    COMPANION_ICON_WATCHFACES,
    COMPANION_ICON_MIMI,
    COMPANION_ICON_CODEX,
    COMPANION_ICON_TRAVEL
};
// 新应用图标放在独立适配层；原有图标集和编号保持自己的单一来源。
// A separate adapter adds new app icons without duplicating the shipped catalog.
lv_obj_t *companion_launcher_icon_create(lv_obj_t *parent, int index);
void companion_launcher_icon_set(lv_obj_t *icon, int index);
