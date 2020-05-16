#pragma once
#include "handmade.h"

struct sim_entity {
	uint32 StorageIndex;

	v2 P;
	uint32 ChunkZ;

	real32 Z;
	real32 dZ;

	entity_type Type;

	world_position P;
	v2 dP;

	real32 Width, Height;

	bool32 Collides;
	int32 dAbsTileZ;

	uint32 HitPointMax;
	hit_point HitPoint[16];

	uint32 SwordLowIndex;
	real32 DistanceRemaining;
	uint32 FacingDirection;

};

struct sim_region {
	world* World;
	world_position Origin;
	rectangle2 Bounds;
	uint32 MaxEntityCount;
	uint32 EntityCount;
	sim_entity *Entities;
};