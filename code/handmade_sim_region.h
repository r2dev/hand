#pragma once
#include "handmade.h"
struct move_spec {
	bool32 UnitMaxAccelVector;
	real32 Speed;
	real32 Drag;
};

enum entity_type {
	EntityType_Null,
	EntityType_Hero,
	EntityType_Wall,
	EntityType_Familiar,
	EntityType_Monster,
	EntityType_Sword
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point {
	uint8 Flag;
	uint8 FilledAmount;
};

enum sim_entity_flags {
	EntityFlag_Collides = (1 << 1),
	EntityFlag_Nonsptial = (1 << 2),
};


struct sim_entity;
union entity_reference {
	sim_entity* Ptr;
	uint32 Index;
};

struct sim_entity {
	uint32 StorageIndex;
	entity_type Type;
	uint32 Flags;

	v2 P;
	v2 dP;

	real32 Z;
	real32 dZ;

	uint32 ChunkZ;

	real32 Width, Height;

	int32 dAbsTileZ;

	uint32 HitPointMax;
	hit_point HitPoint[16];

	entity_reference Sword;
	
	real32 DistanceRemaining;
	uint32 FacingDirection;

};

struct sim_entity_hash {
	uint32 Index;
	sim_entity* Ptr;
};

struct sim_region {
	world* World;
	world_position Origin;
	rectangle2 Bounds;
	uint32 MaxEntityCount;
	uint32 EntityCount;
	sim_entity *Entities;
	sim_entity_hash Hash[4096];
};