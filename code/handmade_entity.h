#pragma once

#define InvalidP v3{100000.0f, 100000.0f, 100000.0f}

struct move_spec {
	b32 UnitMaxAccelVector;
	r32 Speed;
	r32 Drag;
};

enum entity_type {
	EntityType_Null,
    
	EntityType_Floor,
    EntityType_FloatyThingForNow,
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
    EntityFlag_Deleted = (1 << 2),
};

struct entity_id {
    u32 Value;
};
struct entity;

struct entity_reference {
	entity* Ptr;
    entity_id ID;
};

inline b32
ReferencesAreEqual(entity_reference A, entity_reference B) {
    b32 Result = ((A.ID.Value == B.ID.Value) &&
                  (A.Ptr == B.Ptr));
    return(Result);
}

struct entity_collision_volume {
	v3 OffsetP;
	v3 Dim;
};

struct entity_traversable_point {
    v3 P;
    entity *Occupier;
};
struct entity_collision_volume_group {
	entity_collision_volume TotalVolume;
	u32 VolumeCount;
	entity_collision_volume* Volumes;
};

enum entity_movement_code {
    MovementMode_Planted,
    MovementMode_Hopping,
};

struct traversable_reference {
    entity_reference Entity;
    u32 Index;
};

inline b32
IsEqual(traversable_reference A, traversable_reference B) {
    b32 Result = (A.Index == B.Index && 
                  ReferencesAreEqual(A.Entity, B.Entity));
    return(Result);
}

enum entity_visible_piece_flag {
    PieceMove_AxesDeform = 0x1,
    PieceMove_BobOffset = 0x2,
};

struct entity_visible_piece {
    v4 Color;
    r32 Height;
    v3 Offset;
    asset_type_id AssetTypeID;
    u32 Flags;
};

struct entity {
    entity_id ID;
	b32 Updatable;
    ////////////////////////////////////////
    
    
    
    ///////////////////////////////////////
    
    brain_type BrainType;
    brain_slot BrainSlot;
    brain_id BrainID;
    
	u32 Flags;
    
	v3 P;
	v3 dP;
    v3 ddP;  //non-pack
    
	r32 DistanceLimit;
	
	entity_collision_volume_group* Collision;
    
	s32 dAbsTileZ;
    
	u32 HitPointMax;
	hit_point HitPoint[16];
    
	r32 FacingDirection;
    
	//@todo for stairwells
	v2 WalkableDim;
	r32 WalkableHeight;
    
    r32 tBob;
    r32 dtBob;
    r32 ddtBob; //non-pack
    
    move_spec MoveSpec; // non-pack
    
    entity_movement_code MovementMode;
    r32 tMovement;
    traversable_reference Occupying;
    traversable_reference CameFrom;
    
    v2 XAxis;
    v2 YAxis;
    v2 FloorDisplace;
    
    u32 TraversableCount;
    entity_traversable_point Traversables[16];
    
    u32 PieceCount;
    entity_visible_piece Pieces[4];
};

inline b32
IsSet(entity* Entity, u32 Flag) {
	b32 Result = Entity->Flags & Flag;
	return(Result);
}

inline b32
IsDeleted(entity *Entity) {
    b32 Result = IsSet(Entity, EntityFlag_Deleted);
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

