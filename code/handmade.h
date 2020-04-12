#if !defined(HANDMADE_H)
#define HANDMADE_H

#include "handmade_platform.h"

struct memory_arena {
	memory_index Size;
	uint8* Base;
	memory_index Used;
};

#define PushSize(Arena, type) (type*) _PushSize(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type*) _PushSize(Arena, (Count)*sizeof(type))
void*
_PushSize(memory_arena* Arena, memory_index Size) {
	Assert((Arena->Used + Size) <= Arena->Size);
	void* Result = Arena->Base + Arena->Used;
	Arena->Used += Size;
	return(Result);
}

#include "handmade_math.h"
#include "handmade_intrinsics.h"
#include "handmade_tile.h"

struct loaded_bitmap {
	int32 Width;
	int32 Height;
	uint32* Pixels;
};

struct world {
	tile_map* TileMap;
};

struct hero_bitmaps {
	int32 AlignX;
	int32 AlignY;
	loaded_bitmap Head;
	loaded_bitmap Cape;
	loaded_bitmap Torso;
};

struct entity {
	bool32 Exists;
	tile_map_position P;
	v2 dP;
	uint32 FacingDirection;
	real32 Width;
	real32 Height;
};

struct game_state {
	memory_arena WorldArena;
	world* World;

	uint32 CameraFollowingEntityIndex;
	tile_map_position CameraP;

	uint32 PlayerIndexForController[ArrayCount(((game_input*)0)->Controllers)];
	uint32 EntityCount;
	entity Entities[256];



	loaded_bitmap Background;
	
	hero_bitmaps HeroBitmaps[4];
};


#endif
