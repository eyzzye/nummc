#pragma once

// ITEMS
#define UNIT_ITEM_GROUP_NONE        0
#define UNIT_ITEM_GROUP_DAMAGE      1
#define UNIT_ITEM_GROUP_RECOVERY    2
#define UNIT_ITEM_GROUP_SPECIAL     3
#define UNIT_ITEM_GROUP_INVINCIBLE  4
#define UNIT_ITEM_GROUP_BULLET      5
#define UNIT_ITEM_GROUP_ARMOR       6
#define UNIT_ITEM_GROUP_STOCK       7
#define UNIT_ITEM_GROUP_TBOX        8
#define UNIT_ITEM_GROUP_BOM         9
#define UNIT_ITEM_GROUP_END        10

#define UNIT_ITEM_FLAG_DAMAGE      (0x00010000 << UNIT_ITEM_GROUP_DAMAGE)
#define UNIT_ITEM_FLAG_RECOVERY    (0x00010000 << UNIT_ITEM_GROUP_RECOVERY)
#define UNIT_ITEM_FLAG_SPECIAL     (0x00010000 << UNIT_ITEM_GROUP_SPECIAL)
#define UNIT_ITEM_FLAG_INVINCIBLE  (0x00010000 << UNIT_ITEM_GROUP_INVINCIBLE)
#define UNIT_ITEM_FLAG_BULLET      (0x00010000 << UNIT_ITEM_GROUP_BULLET)
#define UNIT_ITEM_FLAG_ARMOR       (0x00010000 << UNIT_ITEM_GROUP_ARMOR)
#define UNIT_ITEM_FLAG_STOCK       (0x00010000 << UNIT_ITEM_GROUP_STOCK)
#define UNIT_ITEM_FLAG_TBOX        (0x00010000 << UNIT_ITEM_GROUP_TBOX)
#define UNIT_ITEM_FLAG_BOM         (0x00010000 << UNIT_ITEM_GROUP_BOM)

#define UNIT_ITEM_ID_IGNORE        (-1)

// RECOVERY
#define UNIT_RECOVERY_ID_NONE              (0)
#define UNIT_RECOVERY_ID_HEART             (1)
#define UNIT_RECOVERY_ID_HEART_UP          (2)
#define UNIT_RECOVERY_ID_HEART_FULL        (3)
#define UNIT_RECOVERY_ID_HEART_FULL_UP     (4)
#define UNIT_RECOVERY_ID_HEART_DOWN        (5)
#define UNIT_RECOVERY_ID_END               (6)

// STOCK_SUB_ID
#define UNIT_STOCK_SUB_ID_NONE             (0)
#define UNIT_STOCK_SUB_ID_WEAPON           (1)
#define UNIT_STOCK_SUB_ID_CHARGE           (2)
#define UNIT_STOCK_SUB_ID_SPECIAL          (3)
#define UNIT_STOCK_SUB_ID_END              (4)

// WEAPON
#define UNIT_WEAPON_ID_NONE                (0)
#define UNIT_WEAPON_ID_BOM                 (1)
#define UNIT_WEAPON_ID_END                 (2)

// CHARGE
#define UNIT_CHARGE_ID_NONE                (0)
#define UNIT_CHARGE_ID_BOOSTER             (1)
#define UNIT_CHARGE_ID_SHIELD              (2)
#define UNIT_CHARGE_ID_HEART               (3)
#define UNIT_CHARGE_ID_BOM                 (4)
#define UNIT_CHARGE_ID_SLOWED              (5)
#define UNIT_CHARGE_ID_RAMPAGE             (6)
#define UNIT_CHARGE_ID_END                 (7)

// SPECIAL
#define UNIT_SPECIAL_ID_NONE                   (0)
#define UNIT_SPECIAL_ID_BOOSTER                (1)
#define UNIT_SPECIAL_ID_BULLET_CURVING_UP      (2)
#define UNIT_SPECIAL_ID_BULLET_RANGE_UP        (3)
#define UNIT_SPECIAL_ID_BULLET_RATE_UP         (4)
#define UNIT_SPECIAL_ID_SHIELD                 (5)
#define UNIT_SPECIAL_ID_SPEED_UP               (6)
#define UNIT_SPECIAL_ID_BULLET_STRENGTH_UP     (7)
#define UNIT_SPECIAL_ID_HEART                  (8)
#define UNIT_SPECIAL_ID_UNKNOWN                (9)
#define UNIT_SPECIAL_ID_BULLET_DOUBLE         (10)
#define UNIT_SPECIAL_ID_BULLET_CURVING_DOWN   (11)
#define UNIT_SPECIAL_ID_BULLET_RANGE_DOWN     (12)
#define UNIT_SPECIAL_ID_BULLET_RATE_DOWN      (13)
#define UNIT_SPECIAL_ID_SPEED_DOWN            (14)
#define UNIT_SPECIAL_ID_BULLET_STRENGTH_DOWN  (15)
#define UNIT_SPECIAL_ID_LUCKY                 (16)
#define UNIT_SPECIAL_ID_GOTO_HELL             (17)
#define UNIT_SPECIAL_ID_SCOPE                 (18)
#define UNIT_SPECIAL_ID_SLOWED                (19)
#define UNIT_SPECIAL_ID_RAMPAGE               (20)
#define UNIT_SPECIAL_ID_END                   (21)

// BULLET
#define UNIT_BULLET_ID_NONE         (0)
#define UNIT_BULLET_ID_POINT        (1)
#define UNIT_BULLET_ID_BIG_POINT    (2)
#define UNIT_BULLET_ID_INVISIBLE    (3)
#define UNIT_BULLET_ID_FIRE         (4)
#define UNIT_BULLET_ID_ICE          (5)
#define UNIT_BULLET_ID_ICE_BALL     (6)
#define UNIT_BULLET_ID_LEASER       (7)
#define UNIT_BULLET_ID_END          (8)

// TRESURE BOX
#define UNIT_TBOX_ID_NONE         (0)
#define UNIT_TBOX_ID_STATIC       (1)
#define UNIT_TBOX_ID_RANDOM       (2)
#define UNIT_TBOX_ID_END          (3)

// BOM
#define UNIT_BOM_ID_NONE         (0)
#define UNIT_BOM_ID_SIMPLE       (1)
#define UNIT_BOM_ID_FALL         (2)
#define UNIT_BOM_ID_EVENT        (3)
#define UNIT_BOM_ID_END          (4)

//
// ALL ITEMS (sid)
//
#define UNIT_ITEM_ID_NONE                  (0)

// DAMAGE
#define UNIT_ITEM_ID_D_TRAP                (UNIT_ITEM_FLAG_DAMAGE     |   1)

// RECOVERY
#define UNIT_ITEM_ID_R_HEART               (UNIT_ITEM_FLAG_RECOVERY   |   UNIT_RECOVERY_ID_HEART           )
#define UNIT_ITEM_ID_R_HEART_UP            (UNIT_ITEM_FLAG_RECOVERY   |   UNIT_RECOVERY_ID_HEART_UP        )
#define UNIT_ITEM_ID_R_HEART_FULL          (UNIT_ITEM_FLAG_RECOVERY   |   UNIT_RECOVERY_ID_HEART_FULL      )
#define UNIT_ITEM_ID_R_HEART_FULL_UP       (UNIT_ITEM_FLAG_RECOVERY   |   UNIT_RECOVERY_ID_HEART_FULL_UP   )
#define UNIT_ITEM_ID_R_HEART_DOWN          (UNIT_ITEM_FLAG_RECOVERY   |   UNIT_RECOVERY_ID_HEART_DOWN      )

// SPECIAL
#define UNIT_ITEM_ID_S_BOOSTER               (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_BOOSTER             )
#define UNIT_ITEM_ID_S_BULLET_CURVING_UP     (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_BULLET_CURVING_UP   )
#define UNIT_ITEM_ID_S_BULLET_RANGE_UP       (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_BULLET_RANGE_UP     )
#define UNIT_ITEM_ID_S_BULLET_RATE_UP        (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_BULLET_RATE_UP      )
#define UNIT_ITEM_ID_S_SHIELD                (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_SHIELD              )
#define UNIT_ITEM_ID_S_SPEED_UP              (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_SPEED_UP            )
#define UNIT_ITEM_ID_S_BULLET_STRENGTH_UP    (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_BULLET_STRENGTH_UP  )
#define UNIT_ITEM_ID_S_HEART                 (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_HEART               )
#define UNIT_ITEM_ID_S_UNKNOWN               (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_UNKNOWN             )
#define UNIT_ITEM_ID_S_BULLET_DOUBLE         (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_BULLET_DOUBLE       )
#define UNIT_ITEM_ID_S_BULLET_CURVING_DOWN   (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_BULLET_CURVING_DOWN )
#define UNIT_ITEM_ID_S_BULLET_RANGE_DOWN     (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_BULLET_RANGE_DOWN   )
#define UNIT_ITEM_ID_S_BULLET_RATE_DOWN      (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_BULLET_RATE_DOWN    )
#define UNIT_ITEM_ID_S_SPEED_DOWN            (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_SPEED_DOWN          )
#define UNIT_ITEM_ID_S_BULLET_STRENGTH_DOWN  (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_BULLET_STRENGTH_DOWN)
#define UNIT_ITEM_ID_S_LUCKY                 (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_LUCKY               )
#define UNIT_ITEM_ID_S_GOTO_HELL             (UNIT_ITEM_FLAG_SPECIAL    |   UNIT_SPECIAL_ID_HELL                )

// INVINCIBLE
#define UNIT_ITEM_ID_I_STAR                (UNIT_ITEM_FLAG_INVINCIBLE |   1)

// BULLET
#define UNIT_ITEM_ID_B_POINT               (UNIT_ITEM_FLAG_BULLET     |   UNIT_BULLET_ID_POINT)
#define UNIT_ITEM_ID_B_BIG_POINT           (UNIT_ITEM_FLAG_BULLET     |   UNIT_BULLET_ID_BIG_POINT)
#define UNIT_ITEM_ID_B_INVISIBLE           (UNIT_ITEM_FLAG_BULLET     |   UNIT_BULLET_ID_INVISIBLE)
#define UNIT_ITEM_ID_B_FIRE                (UNIT_ITEM_FLAG_BULLET     |   UNIT_BULLET_ID_FIRE)
#define UNIT_ITEM_ID_B_ICE                 (UNIT_ITEM_FLAG_BULLET     |   UNIT_BULLET_ID_ICE)
#define UNIT_ITEM_ID_B_LEASER              (UNIT_ITEM_FLAG_BULLET     |   UNIT_BULLET_ID_LEASER)

// ARMOR
#define UNIT_ITEM_ID_A_BARRIER             (UNIT_ITEM_FLAG_ARMOR      |   1)

// STOCK
#define UNIT_ITEM_ID_ST_WEAPON                     (UNIT_ITEM_FLAG_STOCK    | (0x00001000 << UNIT_STOCK_SUB_ID_WEAPON))
#define UNIT_ITEM_ID_ST_WEAPON_BOM                 (UNIT_ITEM_ID_ST_WEAPON  | UNIT_WEAPON_ID_BOM)

#define UNIT_ITEM_ID_ST_CHARGE                     (UNIT_ITEM_FLAG_STOCK    | (0x00001000 << UNIT_STOCK_SUB_ID_CHARGE))
#define UNIT_ITEM_ID_ST_CHARGE_BOOSTER             (UNIT_ITEM_ID_ST_CHARGE  | UNIT_CHARGE_ID_BOOSTER)
#define UNIT_ITEM_ID_ST_CHARGE_SHIELD              (UNIT_ITEM_ID_ST_CHARGE  | UNIT_CHARGE_ID_SHIELD)
#define UNIT_ITEM_ID_ST_CHARGE_HEART               (UNIT_ITEM_ID_ST_CHARGE  | UNIT_CHARGE_ID_HEART)
#define UNIT_ITEM_ID_ST_CHARGE_BOM                 (UNIT_ITEM_ID_ST_CHARGE  | UNIT_CHARGE_ID_BOM)
#define UNIT_ITEM_ID_ST_CHARGE_SLOWED              (UNIT_ITEM_ID_ST_CHARGE  | UNIT_CHARGE_ID_SLOWED)
#define UNIT_ITEM_ID_ST_CHARGE_RAMPAGE             (UNIT_ITEM_ID_ST_CHARGE  | UNIT_CHARGE_ID_RAMPAGE)

#define UNIT_ITEM_ID_ST_SPECIAL                    (UNIT_ITEM_FLAG_STOCK    | (0x00001000 << UNIT_STOCK_SUB_ID_SPECIAL))
#define UNIT_ITEM_ID_ST_SPECIAL_BOOSTER            (UNIT_ITEM_ID_ST_SPECIAL | UNIT_SPECIAL_ID_BOOSTER           )
#define UNIT_ITEM_ID_ST_SPECIAL_BULLET_CURVING_UP  (UNIT_ITEM_ID_ST_SPECIAL | UNIT_SPECIAL_ID_BULLET_CURVING_UP )
#define UNIT_ITEM_ID_ST_SPECIAL_BULLET_RANGE_UP    (UNIT_ITEM_ID_ST_SPECIAL | UNIT_SPECIAL_ID_BULLET_RANGE_UP   )
#define UNIT_ITEM_ID_ST_SPECIAL_BULLET_RATE_UP     (UNIT_ITEM_ID_ST_SPECIAL | UNIT_SPECIAL_ID_BULLET_RATE_UP    )
#define UNIT_ITEM_ID_ST_SPECIAL_SHIELD             (UNIT_ITEM_ID_ST_SPECIAL | UNIT_SPECIAL_ID_SHIELD            )
#define UNIT_ITEM_ID_ST_SPECIAL_SPEED_UP           (UNIT_ITEM_ID_ST_SPECIAL | UNIT_SPECIAL_ID_SPEED_UP          )
#define UNIT_ITEM_ID_ST_SPECIAL_BULLET_STRENGTH_UP (UNIT_ITEM_ID_ST_SPECIAL | UNIT_SPECIAL_ID_BULLET_STRENGTH_UP)
#define UNIT_ITEM_ID_ST_SPECIAL_HEART              (UNIT_ITEM_ID_ST_SPECIAL | UNIT_SPECIAL_ID_HEART             )
#define UNIT_ITEM_ID_ST_SPECIAL_SCOPE              (UNIT_ITEM_ID_ST_SPECIAL | UNIT_SPECIAL_ID_SCOPE             )
#define UNIT_ITEM_ID_ST_SPECIAL_SLOWED             (UNIT_ITEM_ID_ST_SPECIAL | UNIT_SPECIAL_ID_SLOWED            )
#define UNIT_ITEM_ID_ST_SPECIAL_RAMPAGE            (UNIT_ITEM_ID_ST_SPECIAL | UNIT_SPECIAL_ID_RAMPAGE           )

// TRESURE BOX
#define UNIT_ITEM_ID_TB_STATIC             (UNIT_ITEM_FLAG_TBOX       |   UNIT_TBOX_ID_STATIC)
#define UNIT_ITEM_ID_TB_RANDOM             (UNIT_ITEM_FLAG_TBOX       |   UNIT_TBOX_ID_RANDOM)

// BOM
#define UNIT_ITEM_ID_BM_SIMPLE             (UNIT_ITEM_FLAG_BOM        |   UNIT_BOM_ID_SIMPLE)
#define UNIT_ITEM_ID_BM_FALL               (UNIT_ITEM_FLAG_BOM        |   UNIT_BOM_ID_FALL)
#define UNIT_ITEM_ID_BM_EVENT              (UNIT_ITEM_FLAG_BOM        |   UNIT_BOM_ID_EVENT)
