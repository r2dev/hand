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

inline void
InitializeArena(memory_arena* Arena, memory_index Size, void* Storage) {
	Arena->Size = Size;
	Arena->Base = (uint8*)Storage;
	Arena->Used = 0;
}


#define PushStruct(Arena, type) (type*) _PushSize(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type*) _PushSize(Arena, (Count)*sizeof(type))
inline void*
_PushSize(memory_arena* Arena, memory_index Size) {
	Assert((Arena->Used + Size) <= Arena->Size);
	void* Result = Arena->Base + Arena->Used;
	Arena->Used += Size;
	return(Result);
}
#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
inline void
ZeroSize(memory_index Size, void* Ptr) {
	uint8* Byte = (uint8 *)Ptr;
	while (Size--) {
		*Byte = 0;
	}
}

#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "handmade_world.h"
#include "handmade_sim_region.h"
#include "handmade_entity.h"

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


struct low_entity {
	world_position P;
	sim_entity Sim;
};

struct controlled_hero {
	uint32 EntityIndex;
	v2 ddP;
	v2 dSword;
	real32 dZ;
};


struct game_state {
	memory_arena WorldArena;
	world* World;

	uint32 CameraFollowingEntityIndex;
	world_position CameraP;

	controlled_hero ControlledHeroes[ArrayCount(((game_input*)0)->Controllers)];

	

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

inline low_entity*
GetLowEntity(game_state* GameState, uint32 Index) {
	low_entity* Result = 0;

	if ((Index > 0) && (Index < GameState->LowEntityCount)) {
		Result = GameState->LowEntities + Index;
	}
	return(Result);
}

bool32
TestWall(real32 WallX, real32 RelX, real32 RelY,
	real32 PlayerDeltaX, real32 PlayerDeltaY, real32* tMin, real32 MinY, real32 MaxY) {
	real32 tEpsilon = 0.001f;
	bool32 Hit = false;
	if (PlayerDeltaX != 0.0f) {
		real32 tResult = (WallX - RelX) / PlayerDeltaX;
		real32 Y = RelY + tResult * PlayerDeltaY;
		if ((tResult >= 0.0f) && (*tMin > tResult)) {
			if ((Y >= MinY) && (Y <= MaxY)) {
				Hit = true;
				*tMin = Maximum(0.0f, tResult - tEpsilon);
			}
		}
	}
	return(Hit);
}

#endif
