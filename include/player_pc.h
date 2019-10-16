#ifndef GUARD_PLAYER_PC_H
#define GUARD_PLAYER_PC_H

#include "global.h"
#include "list_menu.h"

// Exported type declarations

struct PlayerPCItemPageStruct
{
    u16 selectedRow;
    u16 scrollOffset;
    u8 pageItems;
    u8 count;
    u8 filler_6[3];
    u8 unk_9;
    u8 scrollIndicatorId;
    u8 filler_B[5];
};

// Exported RAM declarations
extern struct PlayerPCItemPageStruct gPlayerPcMenuManager;

// Exported ROM declarations

void sub_816B060(u8 taskId);
void NewGameInitPCItems(void);

#endif //GUARD_PLAYER_PC_H
