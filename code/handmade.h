#if !defined(HANDMADE_H)
#define HANDMADE_H

#include "handmade_platform.h"

#define Minimum(A, B) ((A < B)? (A): (B))
#define Maximum(A, B) ((A > B)? (A): (B))

struct memory_arena {
	memory_index Size;
	uint8* Base;
	memory_index Used;

	int32 TempCount;
};

struct temporary_memory {
	memory_arena* Arena;
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
#define PushSize(Arena, Size) _PushSize(Arena, Size)
inline void*
_PushSize(memory_arena* Arena, memory_index Size) {
	Assert((Arena->Used + Size) <= Arena->Size);
	void* Result = Arena->Base + Arena->Used;
	Arena->Used += Size;
	return(Result);
}


inline temporary_memory
BeginTemporaryMemory(memory_arena* Arena) {
	temporary_memory Result;
	Result.Arena = Arena;
	Result.Used = Arena->Used;
	Arena->TempCount++;
	return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempMem) {
	memory_arena* Arena = TempMem.Arena;
	Assert(Arena->Used >= TempMem.Used)
	Arena->Used = TempMem.Used;
	Assert(Arena->TempCount > 0)
	--Arena->TempCount;
}

inline void
CheckArena(memory_arena* Arena) {
	Assert(Arena->TempCount == 0);
}


#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
inline void
ZeroSize(memory_index Size, void* Ptr) {
	uint8* Byte = (uint8 *)Ptr;
	while(Size--) {
		*Byte++ = 0;  // should be *Byte++ = 0
	}
}

#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "handmade_world.h"
#include "handmade_sim_region.h"
#include "handmade_entity.h"
#include "handmade_render_group.h"


struct hero_bitmaps {
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

struct pairwise_collision_rule {
	bool32 CanCollide;
	uint32 StorageIndexA;
	uint32 StorageIndexB;

	pairwise_collision_rule* NextHash;
};

struct ground_buffer {
	world_position P;
	loaded_bitmap Bitmap;
};

struct game_state;
internal void AddCollisionRule(game_state* GameState, uint32 StorageIndexA, uint32 StorageIndexB, bool32 ShouldCollide);
internal void ClearCollisionRulesFor(game_state* GameState, uint32 StorageIndex);

struct game_state {
	memory_arena WorldArena;
	world* World;

	uint32 CameraFollowingEntityIndex;
	world_position CameraP;

	controlled_hero ControlledHeroes[ArrayCount(((game_input*)0)->Controllers)];

	real32 TypicalFloorHeight = 3.0f;

	uint32 LowEntityCount;
	low_entity LowEntities[100000];

	loaded_bitmap Grass[2];
	loaded_bitmap Ground[4];
	loaded_bitmap Tuft[3];


	loaded_bitmap Background;
	loaded_bitmap Shadow;
	loaded_bitmap Tree;
	loaded_bitmap Sword;

	loaded_bitmap Stairwell;

	hero_bitmaps HeroBitmaps[4];

	pairwise_collision_rule* CollisionRuleHash[256];
	pairwise_collision_rule* FirstFreeCollisionRule;

	sim_entity_collision_volume_group* NullCollision;
	sim_entity_collision_volume_group* SwordCollision;
	sim_entity_collision_volume_group* StairCollision;
	sim_entity_collision_volume_group* PlayerCollision;
	sim_entity_collision_volume_group* MonstarCollision;
	sim_entity_collision_volume_group* WallCollision;
	sim_entity_collision_volume_group* FamiliarCollision;
	sim_entity_collision_volume_group* StandardRoomCollision;

	real32 time;

	loaded_bitmap TestDiffuse;
	loaded_bitmap TestNormal;
};

struct transient_state {
	bool32 IsInitialized;
	memory_arena TranArena;

	uint32 GroundBufferCount;
	ground_buffer* GroundBuffers;


	uint32 EnvMapWidth;
	uint32 EnvMapHeight;
	environment_map EnvMaps[3];

	platform_work_queue* RenderQueue;

	
};

inline low_entity*
GetLowEntity(game_state* GameState, uint32 Index) {
	low_entity* Result = 0;

	if ((Index > 0) && (Index < GameState->LowEntityCount)) {
		Result = GameState->LowEntities + Index;
	}
	return(Result);
}

global_variable platform_add_entry* PlatformAddEntry;
global_variable platform_complete_all_work* PlatformCompleteAllWork;


#endif
