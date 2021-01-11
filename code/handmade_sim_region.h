#ifndef HANDMADE_SIM_REGION_H
#define HANDMADE_SIM_REGION_H
#include "handmade.h"
Introspect(category: "sim") struct move_spec {
	b32 UnitMaxAccelVector;
	r32 Speed;
	r32 Drag;
};

enum entity_type {
	EntityType_Null,
    
	EntityType_Space,
    
	EntityType_Hero,
	EntityType_Wall,
	EntityType_Familiar,
	EntityType_Monster,
	EntityType_Sword,
	EntityType_Stairwell,
};

#define HIT_POINT_SUB_COUNT 4
Introspect(category: "sim") struct hit_point {
	u8 Flag;
	u8 FilledAmount;
};

enum sim_entity_flags {
	EntityFlag_Collides = (1 << 0),
	EntityFlag_Nonspatial = (1 << 1),
	EntityFlag_Moveable = (1 << 2),
	EntityFlag_ZSupported = (1 << 3),
	EntityFlag_Traversable = (1 << 4),
	EntityFlag_Simming = (1 << 30),
};


struct sim_entity;

Introspect(category: "sim")
union entity_reference {
	sim_entity* Ptr;
	u32 Index;
};


Introspect(category: "sim")
struct sim_entity_collision_volume {
	v3 OffsetP;
	v3 Dim;
};

Introspect(category: "sim")
struct sim_entity_collision_volume_group {
	sim_entity_collision_volume TotalVolume;
	u32 VolumeCount;
	sim_entity_collision_volume* Volumes;
};

Introspect(category: "sim")
struct sim_entity {
	u32 StorageIndex;
	b32 Updatable;
    
	entity_type Type;
	u32 Flags;
    
	v3 P;
	v3 dP;
    
	r32 DistanceLimit;
	
	sim_entity_collision_volume_group* Collision;
    
	s32 dAbsTileZ;
    
	u32 HitPointMax;
	hit_point HitPoint[16];
    
	entity_reference Sword;
	
	r32 FacingDirection;
    
	//@todo for stairwells
	v2 WalkableDim;
	r32 WalkableHeight;
    
};

struct sim_entity_hash {
	u32 Index;
	sim_entity* Ptr;
};

Introspect(category: "sim") struct sim_region {
	world* World;
    
	r32 MaxEntityRadius;
	r32 MaxEntityVelocity;
    
	world_position Origin;
	rectangle3 Bounds;
	rectangle3 UpdatableBounds;
	u32 MaxEntityCount;
	u32 EntityCount;
	sim_entity *Entities;
	sim_entity_hash Hash[4096];
};
#endif