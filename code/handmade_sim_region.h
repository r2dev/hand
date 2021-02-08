#ifndef HANDMADE_SIM_REGION_H
#define HANDMADE_SIM_REGION_H
struct move_spec {
	b32 UnitMaxAccelVector;
	r32 Speed;
	r32 Drag;
};

struct entity_hash {
    entity_id ID;
	entity* Ptr;
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
	entity_hash Hash[4096];
};
#endif