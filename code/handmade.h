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

struct game_state {
	memory_arena WorldArena;
	world* World;

	tile_map_position CameraP;
	tile_map_position PlayerP;

	loaded_bitmap Background;
	uint32 HeroFacingDirection;
	hero_bitmaps HeroBitmaps[4];
};


#endif
