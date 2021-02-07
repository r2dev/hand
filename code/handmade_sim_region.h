#ifndef HANDMADE_SIM_REGION_H
#define HANDMADE_SIM_REGION_H
Introspect(category: "sim") struct move_spec {
	b32 UnitMaxAccelVector;
	r32 Speed;
	r32 Drag;
};

enum entity_type {
	EntityType_Null,
    
	EntityType_Floor,
    
	EntityType_HeroBody,
    EntityType_HeroHead,
	EntityType_Wall,
	EntityType_Familiar,
	EntityType_Monster,
	EntityType_Stairwell,
};

#define HIT_POINT_SUB_COUNT 4
Introspect(category: "sim") struct hit_point {
	u8 Flag;
	u8 FilledAmount;
};

enum entity_flags {
	EntityFlag_Collides = (1 << 0),
	EntityFlag_Moveable = (1 << 1),
	EntityFlag_Simming = (1 << 30),
};

struct entity_id {
    u32 Value;
};
struct entity;
Introspect(category: "sim")
union entity_reference {
	entity* Ptr;
    entity_id ID;
};


Introspect(category: "sim")
struct entity_collision_volume {
	v3 OffsetP;
	v3 Dim;
};

struct entity_traversable_point {
    v3 P;
};
struct entity_collision_volume_group {
	entity_collision_volume TotalVolume;
	u32 VolumeCount;
	entity_collision_volume* Volumes;
    u32 TraversableCount;
    entity_traversable_point *Traversables;
};

enum sim_movement_code {
    MovementMode_Planted,
    MovementMode_Hopping,
};

struct entity {
    world_position ChunkP;
    entity_id ID;
	b32 Updatable;
    
	entity_type Type;
	u32 Flags;
    
	v3 P;
	v3 dP;
    
	r32 DistanceLimit;
	
	entity_collision_volume_group* Collision;
    
	s32 dAbsTileZ;
    
	u32 HitPointMax;
	hit_point HitPoint[16];
    
    entity_reference Head;
	
	r32 FacingDirection;
    
	//@todo for stairwells
	v2 WalkableDim;
	r32 WalkableHeight;
    
    r32 tBob;
    r32 dtBob;
    
    sim_movement_code MovementMode;
    r32 tMovement;
    v3 MovementFrom;
    v3 MovementTo;
    
    v2 XAxis;
    v2 YAxis;
    v2 FloorDisplace;
};

struct entity_hash {
    entity_id ID;
	entity* Ptr;
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
	entity *Entities;
	entity_hash Hash[4096];
};
#endif