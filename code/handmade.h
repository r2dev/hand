#if !defined(HANDMADE_H)
#define HANDMADE_H

#include "handmade_platform.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"
#include "handmade_file_formats.h"
#include "zha_data_structure.h"
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

#define Minimum(A, B) ((A < B)? (A): (B))
#define Maximum(A, B) ((A > B)? (A): (B))
inline b32
StringsAreEqual(char* A, char* B) {
    while (*A && *B &&  *A++ == *B++) {
        
    }
    b32 Result = (*A == 0 && *B == 0);
    return(Result);
}

inline void
InitializeArena(memory_arena* Arena, memory_index Size, void* Storage) {
	Arena->Size = Size;
	Arena->Base = (uint8*)Storage;
	Arena->Used = 0;
}

inline memory_index
GetAlignmentOffset(memory_arena* Arena, memory_index Alignment = 4) {
	memory_index Offset = 0;
	memory_index ResultPointer = (memory_index)Arena->Base + Arena->Used;
	memory_index AlignmentMask = Alignment - 1;
	if (ResultPointer & AlignmentMask) {
		Offset = Alignment - (ResultPointer & AlignmentMask);
	}
	return(Offset);
}


inline memory_index
GetArenaSizeRemaining(memory_arena* Arena, memory_index Alignment = 4) {
	memory_index Result = Arena->Size - (Arena->Used + GetAlignmentOffset(Arena, Alignment));
	return(Result);
}


#define PushStruct(Arena, type, ...) (type*) _PushSize(Arena, sizeof(type), ## __VA_ARGS__)
#define PushArray(Arena, Count, type, ...) (type*) _PushSize(Arena, (Count)*sizeof(type), ## __VA_ARGS__)
#define PushSize(Arena, Size, ...) _PushSize(Arena, Size, ## __VA_ARGS__)
#define PushCopy(Arena, Size, Source, ...) Copy(Size, Source, _PushSize(Arena, Size, ## __VA_ARGS__))
inline void*
_PushSize(memory_arena* Arena, memory_index Size, memory_index Alignment = 4) {
    
	memory_index Pointer = (memory_index)Arena->Base + Arena->Used;
	memory_index AlignmentMask = Alignment - 1;
	memory_index Offset = GetAlignmentOffset(Arena, Alignment);
	Size += Offset;
	Assert((Arena->Used + Size) <= Arena->Size);
	Arena->Used += Size;
    
	void* Result = (void*)(Pointer + Offset);
	
	return(Result);
}

inline char*
PushString(memory_arena* Arena, char* Content) {
	uint32 Size = 1;
	for (char* At = Content; *At; ++At) {
		++Size;
	}
    
	char* Dest = (char*)(_PushSize(Arena, Size));
	for (u32 CharIndex = 0; CharIndex < Size; ++CharIndex) {
		Dest[CharIndex] = Content[CharIndex];
	}
	return(Dest);
}

inline void 
SubArena(memory_arena* Result, memory_arena *Arena, memory_index Size, memory_index Alignment = 4) {
	Result->Size = Size;
	Result->Base = (uint8*)_PushSize(Arena, Size, Alignment);
	Result->TempCount = 0;
	Result->Used = 0;
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
#define ZeroArray(Count, Pointer) ZeroSize(Count * sizeof((Pointer)[0]), Pointer)
inline void
ZeroSize(memory_index Size, void* Ptr) {
	uint8* Byte = (uint8 *)Ptr;
	while(Size--) {
		*Byte++ = 0;  // should be *Byte++ = 0
	}
}

inline void*
Copy(memory_index Size, void* SourceInit, void* DestInit) {
    u8* Source = (u8 *)SourceInit;
    u8* Dest = (u8 *)DestInit;
    while (Size--) {*Dest++ = *Source++;}
    return(DestInit);
}



#include "handmade_random.h"
#include "handmade_asset.h"
#include "handmade_world.h"
#include "handmade_sim_region.h"
#include "handmade_entity.h"
#include "handmade_render_group.h"
#include "handmade_audio.h"

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

struct particle_cel {
    r32 Density;
    v3 VelocityTimesDensity;
};

struct particle {
    v3 P;
    v3 dP;
    v3 ddP;
    v4 Color;
    v4 dColor;
};

struct game_state;
internal void AddCollisionRule(game_state* GameState, uint32 StorageIndexA, uint32 StorageIndexB, bool32 ShouldCollide);
internal void ClearCollisionRulesFor(game_state* GameState, uint32 StorageIndex);

struct game_state {
	bool32 IsInitialized;
	memory_arena WorldArena;
	world* World;
    
	uint32 CameraFollowingEntityIndex;
	world_position CameraP;
    
	controlled_hero ControlledHeroes[ArrayCount(((game_input*)0)->Controllers)];
    
	real32 TypicalFloorHeight = 3.0f;
    
	uint32 LowEntityCount;
	low_entity LowEntities[100000];
    
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
    
    u32 NextParticle;
    particle Particle[256];
#define PARTICEL_CEL_DIM 16
    particle_cel ParticleCels[PARTICEL_CEL_DIM][PARTICEL_CEL_DIM];
    
    random_series EffectEntropy;
    
	loaded_bitmap TestDiffuse;
	loaded_bitmap TestNormal;
	audio_state AudioState;
    
	playing_sound* Music;
};

struct task_with_memory {
	bool32 BeingUsed;
	memory_arena Arena;
	temporary_memory MemoryFlush;
};


struct transient_state {
	bool32 IsInitialized;
	memory_arena TranArena;
    
	task_with_memory Tasks[4];
	uint32 GroundBufferCount;
	ground_buffer* GroundBuffers;
    
	game_assets* Assets;
    
    
	uint32 EnvMapWidth;
	uint32 EnvMapHeight;
	environment_map EnvMaps[3];
    
	platform_work_queue* HighPriorityQueue;
	platform_work_queue* LowPriorityQueue;
};

inline low_entity*
GetLowEntity(game_state* GameState, uint32 Index) {
	low_entity* Result = 0;
    
	if ((Index > 0) && (Index < GameState->LowEntityCount)) {
		Result = GameState->LowEntities + Index;
	}
	return(Result);
}


internal task_with_memory* BeginTaskWithMemory(transient_state* TranState);
inline void EndTaskWithMemory(task_with_memory* Task);

global_variable platform_api Platform;

#endif
