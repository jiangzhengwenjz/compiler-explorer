#ifndef GUARD_FLDEFF_H
#define GUARD_FLDEFF_H

#define FLDEFF_CALL_FUNC_IN_DATA() ((void (*)(void))(((u16)gTasks[taskId].data[8] << 16) | (u16)gTasks[taskId].data[9]))();

#define FLDEFF_SET_FUNC_TO_DATA(func)                     \
gTasks[taskId].data[8] = (u32)func >> 16;                 \
gTasks[taskId].data[9] = (u32)func;

extern u8 *gUnknown_203AAB0;
extern struct MapPosition gPlayerFacingPosition;

bool8 CheckObjectGraphicsInFrontOfPlayer(u8 graphicsId);
u8 oei_task_add(void);

// flash
u8 sub_80C9DCC(u8 lightLevel, u8 mapType);
u8 sub_80C9D7C(u8 mapType1, u8 mapType2);

// cut

// dig
bool8 SetUpFieldMove_Dig(void);
bool8 FldEff_UseDig(void);

// rocksmash
bool8 SetUpFieldMove_RockSmash(void);
bool8 FldEff_UseRockSmash(void);

// berrytree
void nullsub_56(void);

// poison
void FldEffPoison_Start(void);
bool32 FldEffPoison_IsActive(void);

// strength
bool8 SetUpFieldMove_Strength(void);
bool8 sub_80D0860(void);

// teleport
bool8 SetUpFieldMove_Teleport(void);
bool8 FldEff_UseTeleport(void);

// softboiled
bool8 hm_prepare_dive_probably(void);
void sub_80E56DC(u8 taskId);
void sub_80E5724(u8 taskId);

// sweetscent
bool8 SetUpFieldMove_SweetScent(void);
bool8 FldEff_SweetScent(void);

#endif // GUARD_FLDEFF_H
