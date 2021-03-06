#pragma once

// EFFECT
#define UNIT_EFFECT_GROUP_NONE      0
#define UNIT_EFFECT_GROUP_STATIC    1
#define UNIT_EFFECT_GROUP_DYNAMIC   2
#define UNIT_EFFECT_GROUP_DIE_AUTO  3
#define UNIT_EFFECT_GROUP_END       4

#define UNIT_EFFECT_CLEAR_TYPE_NONE           0
#define UNIT_EFFECT_CLEAR_TYPE_KEEP_ON_STAGE  1
#define UNIT_EFFECT_CLEAR_TYPE_END            2

#define UNIT_EFFECT_ID_NONE             0
#define UNIT_EFFECT_ID_FIRE_UP          1
#define UNIT_EFFECT_ID_FREEZE_UP        2
#define UNIT_EFFECT_ID_BOOST            3
#define UNIT_EFFECT_ID_SHIELD           4
#define UNIT_EFFECT_ID_SMOKE            5
#define UNIT_EFFECT_ID_TRASH            6
#define UNIT_EFFECT_ID_HIGH_LIGHT_LINE  7
#define UNIT_EFFECT_ID_END              8

#define UNIT_EFFECT_ID_IGNORE           (-1)

// for player
#define UNIT_EFFECT_ID_P_NONE         (0)
#define UNIT_EFFECT_ID_P_FIRE_UP      (1)
#define UNIT_EFFECT_ID_P_FREEZE_UP    (2)
#define UNIT_EFFECT_ID_P_BOOST        (3)
#define UNIT_EFFECT_ID_P_SHIELD       (4)
#define UNIT_EFFECT_ID_P_RAMPAGE      (5)
#define UNIT_EFFECT_ID_P_END          (6)

#define UNIT_EFFECT_FLAG_P_NONE       (0)
#define UNIT_EFFECT_FLAG_P_FIRE_UP    (0x00000001 << UNIT_EFFECT_ID_P_FIRE_UP)
#define UNIT_EFFECT_FLAG_P_FREEZE_UP  (0x00000001 << UNIT_EFFECT_ID_P_FREEZE_UP)
#define UNIT_EFFECT_FLAG_P_BOOST      (0x00000001 << UNIT_EFFECT_ID_P_BOOST)
#define UNIT_EFFECT_FLAG_P_SHIELD     (0x00000001 << UNIT_EFFECT_ID_P_SHIELD)
#define UNIT_EFFECT_FLAG_P_RAMPAGE    (0x00000001 << UNIT_EFFECT_ID_P_RAMPAGE)

#define UNIT_EFFECT_FLAG_P_NUM_MAX          (8) /* effect + stat only */

// for enemy (effect)
#define UNIT_EFFECT_ID_E_NONE               (0)
#define UNIT_EFFECT_ID_E_FIRE_UP            (1)
#define UNIT_EFFECT_ID_E_FREEZE_UP          (2)
#define UNIT_EFFECT_ID_E_END                (3)

#define UNIT_EFFECT_FLAG_E_NONE             (0)
#define UNIT_EFFECT_FLAG_E_FIRE_UP          (0x00000001 << UNIT_EFFECT_ID_E_FIRE_UP)
#define UNIT_EFFECT_FLAG_E_FREEZE_UP        (0x00000001 << UNIT_EFFECT_ID_E_FREEZE_UP)

// for enemy (stat only)
#define UNIT_EFFECT_ID_E_NO_FRICTION        (16)
#define UNIT_EFFECT_ID_E_NO_TRAP_DAMAGE     (17)
#define UNIT_EFFECT_FLAG_E_NO_FRICTION      (0x00010000 << (UNIT_EFFECT_ID_E_NO_FRICTION - 16))
#define UNIT_EFFECT_FLAG_E_NO_TRAP_DAMAGE   (0x00010000 << (UNIT_EFFECT_ID_E_NO_TRAP_DAMAGE - 16))

#define UNIT_EFFECT_FLAG_E_NUM_MAX          (8) /* effect + stat only */
