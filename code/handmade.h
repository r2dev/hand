#if !defined(HANDMADE_H)
#define HANDMADE_H

#include "handmade_platform.h"

#define Minimum(A, B) ((A < B)? (A): (B))
#define Maximum(A, B) ((A > B)? (A): (B))

struct memory_arena {
	memory_index Size;
	uint8* Base;
	memory_index Used;
};

#define PushStruct(Arena, type) (type*) _PushSize(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type*) _PushSize(Arena, (Count)*sizeof(type))
void*
_PushSize(memory_arena* Arena, memory_index Size) {
	Assert((Arena->Used + Size) <= Arena->Size);
	void* Result = Arena->Base + Arena->Used;
	Arena->Used += Size;
	return(Result);
}

#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "handmade_world.h"
#include "handmade_sim_region.h"

struct loaded_bitmap {
	int32 Width;
	int32 Height;
	uint32* Pixels;
};

struct hero_bitmaps {
	v2 Align;
	loaded_bitmap Head;
	loaded_bitmap Cape;
	loaded_bitmap Torso;
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point {
	uint8 Flag;
	uint8 FilledAmount;
};


enum entity_type {
	EntityType_Null,
	EntityType_Hero,
	EntityType_Wall,
	EntityType_Familiar,
	EntityType_Monster,
	EntityType_Sword
};

struct low_entity {
	sim_entity Sim;
};


struct game_state {
	memory_arena WorldArena;
	world* World;

	uint32 CameraFollowingEntityIndex;
	world_position CameraP;

	uint32 PlayerIndexForController[ArrayCount(((game_input*)0)->Controllers)];

	

	uint32 LowEntityCount;
	low_entity LowEntities[100000];

	loaded_bitmap Background;
	loaded_bitmap Shadow;
	loaded_bitmap Tree;
	loaded_bitmap Sword;

	hero_bitmaps HeroBitmaps[4];
	real32 MetersToPixels;
};

struct entity_visible_piece {
	loaded_bitmap* Bitmap;
	v2 Offset;
	real32 OffsetZ;
	real32 R, G, B, A;
	v2 Dim;
	real32 EntityZC;
};

struct entity_visible_piece_group {
	game_state* GameState;
	uint32 PieceCount;
	entity_visible_piece Pieces[8];
};

#endif
