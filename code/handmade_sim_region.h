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
	EntityType_Sword,
	EntityType_Stairwell
};

#define HIT_POINT_SUB_COUNT 4
struct hit_point {
	uint8 Flag;
	uint8 FilledAmount;
};

enum sim_entity_flags {
	EntityFlag_Collides = (1 << 0),
	EntityFlag_Nonspatial = (1 << 1),
	EntityFlag_Moveable = (1 << 2),
	EntityFlag_ZSupported = (1 << 4),
	EntityFlag_Simming = (1 << 30),
};


struct sim_entity;
union entity_reference {
	sim_entity* Ptr;
	uint32 Index;
};


struct sim_entity_collision_volume {
	v3 OffsetP;
	v3 Dim;
};

struct sim_entity_collision_volume_group {
	sim_entity_collision_volume TotalVolume;
	uint32 VolumeCount;
	sim_entity_collision_volume* Volumes;
};

struct sim_entity {
	uint32 StorageIndex;
	bool32 Updatable;

	entity_type Type;
	uint32 Flags;

	v3 P;
	v3 dP;

	real32 DistanceLimit;
	
	sim_entity_collision_volume_group* Collision;

	int32 dAbsTileZ;

	uint32 HitPointMax;
	hit_point HitPoint[16];

	entity_reference Sword;
	
	uint32 FacingDirection;

	//@todo for stairwells
	v2 WalkableDim;
	real32 WalkableHeight;

};

struct sim_entity_hash {
	uint32 Index;
	sim_entity* Ptr;
};

struct sim_region {
	world* World;

	real32 MaxEntityRadius;
	real32 MaxEntityVelocity;

	world_position Origin;
	rectangle3 Bounds;
	rectangle3 UpdatableBounds;
	uint32 MaxEntityCount;
	uint32 EntityCount;
	sim_entity *Entities;
	sim_entity_hash Hash[4096];
};