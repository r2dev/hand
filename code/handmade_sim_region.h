#ifndef HANDMADE_SIM_REGION_H
#define HANDMADE_SIM_REGION_H
struct move_spec {
	b32 UnitMaxAccelVector;
	r32 Speed;
	r32 Drag;
};

struct entity_hash {
	entity* Ptr;
    entity_id ID;
};

struct brain_hash {
    brain *Ptr;
    brain_id ID;
};

struct sim_region {
	world* World;
    
	r32 MaxEntityRadius;
	r32 MaxEntityVelocity;
    
	world_position Origin;
	rectangle3 Bounds;
	rectangle3 UpdatableBounds;
	u32 MaxEntityCount;
	u32 EntityCount;
	entity *Entities;
	entity_hash EntityHash[4096];
    
    u32 MaxBrainCount;
    u32 BrainCount;
    brain *Brains;
    brain_hash BrainHash[256];
};

internal entity_hash *GetHashFromID(sim_region* SimRegion, entity_id ID);
#endif