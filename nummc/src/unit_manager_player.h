#pragma once

// PLAYER
#define UNIT_PLAYER_STAT_INVINCIBLE_TIMER  2000

#define UNIT_PLAYER_HP_MAX_VAL_MIN            0
#define UNIT_PLAYER_HP_MAX_VAL_MAX          120

// rate = (X * luck_base + (luck_base / 2)) / luck
#define UNIT_PLAYER_LUCK_BASE_NUM           (8)
#define UNIT_PLAYER_LUCK_VAL(_X,_LUCK)  ((_X * UNIT_PLAYER_LUCK_BASE_NUM + (UNIT_PLAYER_LUCK_BASE_NUM / 2)) / _LUCK)
