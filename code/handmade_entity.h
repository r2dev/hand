#pragma once

#define InvalidP v3{100000.0f, 100000.0f, 100000.0f}


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
struct hit_point {
	u8 Flag;
	u8 FilledAmount;
};

enum entity_flags {
	EntityFlag_Collides = (1 << 0),
	EntityFlag_Moveable = (1 << 1),
};

struct entity_id {
    u32 Value;
};
struct entity;

union entity_reference {
	entity* Ptr;
    entity_id ID;
};

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

enum entity_movement_code {
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
    
    entity_movement_code MovementMode;
    r32 tMovement;
    v3 MovementFrom;
    v3 MovementTo;
    
    v2 XAxis;
    v2 YAxis;
    v2 FloorDisplace;
};

inline b32
IsSet(entity* Entity, u32 Flag) {
	b32 Result = Entity->Flags & Flag;
	return(Result);
}

inline void
AddFlags(entity* Entity, u32 Flags) {
	Entity->Flags |= Flags;
}

inline void
ClearFlags(entity* Entity, u32 Flags) {
	Entity->Flags &= ~Flags;
}

internal v3
GetEntityGroundPoint(entity* Entity, v3 ForEntityP) {
	return(ForEntityP);
}

internal v3
GetEntityGroundPoint(entity* Entity) {
	v3 Result = Entity->P;
	return(Result);
}

inline r32
GetStairGround(entity* Entity, v3 AtGroundPoint) {
	Assert(Entity->Type == EntityType_Stairwell);
	rectangle2 RegionRect = RectCenterDim(Entity->P.xy, Entity->WalkableDim);
	v2 Bary = Clamp01(GetBarycentric(RegionRect, AtGroundPoint.xy));
	r32 Result = Entity->P.z + Bary.y * Entity->WalkableHeight;
	return(Result);
}