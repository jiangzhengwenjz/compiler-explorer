#ifndef GUARD_BATTLE_SETUP_H
#define GUARD_BATTLE_SETUP_H

#include "global.h"

void BattleSetup_StartScriptedWildBattle(void);
u8 BattleSetup_GetTerrainId(void);
u8 *BattleSetup_ConfigureTrainerBattle(const u8 *data);
void BattleSetup_StartBattlePikeWildBattle(void);
void BattleSetup_StartWildBattle(void);
void BattleSetup_StartRoamerBattle(void);

u8 HasTrainerAlreadyBeenFought(u16);
void SetTrainerFlag(u16);
void ClearTrainerFlag(u16);
void BattleSetup_StartTrainerBattle(void);
u8 *BattleSetup_GetScriptAddrAfterBattle(void);
u8 *BattleSetup_GetTrainerPostBattleScript(void);
void sub_80803FC(void);
u8 sub_8080060(void);

#endif // GUARD_BATTLE_SETUP_H
