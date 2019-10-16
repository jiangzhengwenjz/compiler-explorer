#ifndef GUARD_FIELDMAP_H
#define GUARD_FIELDMAP_H

#include "global.h"

#define NUM_TILES_IN_PRIMARY 640
#define NUM_TILES_TOTAL 1024
#define NUM_METATILES_IN_PRIMARY 640
#define NUM_METATILES_TOTAL 1024
#define NUM_PALS_IN_PRIMARY 7
#define NUM_PALS_TOTAL 13
#define VIRTUAL_MAP_SIZE 0x2800

extern struct BackupMapData VMap;
extern const struct MapData Route1_Layout;

u32 MapGridGetMetatileIdAt(int, int);
u32 MapGridGetMetatileBehaviorAt(int, int);
void MapGridSetMetatileIdAt(int, int, u16);
void MapGridSetMetatileEntryAt(int, int, u16);
void GetCameraCoords(u16*, u16*);
bool8 MapGridIsImpassableAt(s32, s32);
s32 GetMapBorderIdAt(s32, s32);
bool32 CanCameraMoveInDirection(s32);
u32 GetBehaviorByMetatileIdAndMapData(struct MapData *mapData, u16 metatile, u8 attr);
const struct MapHeader * mapconnection_get_mapheader(struct MapConnection * connection);
struct MapConnection * GetMapConnectionAtPos(s16 x, s16 y);

void save_serialize_map(void);

#endif //GUARD_FIELDMAP_H
