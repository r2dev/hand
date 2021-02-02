/* date = January 24th 2021 6:07 pm */

#ifndef HANDMADE_WORLD_MODE_H
#define HANDMADE_WORLD_MODE_H

struct low_entity {
    world_position P;
	sim_entity Sim;
};

struct pairwise_collision_rule {
	b32 CanCollide;
	u32 StorageIndexA;
	u32 StorageIndexB;
    
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

struct game_mode_world;
internal void AddCollisionRule(game_mode_world *GameWorld, u32 StorageIndexA, u32 StorageIndexB, b32 ShouldCollide);
internal void ClearCollisionRulesFor(game_mode_world *GameWorld, u32 StorageIndex);

struct game_mode_world {
    
    world* World;
    
    u32 CameraFollowingEntityIndex;
	world_position CameraP;
    
	r32 TypicalFloorHeight = 3.0f;
    
	u32 LowEntityCount;
	low_entity LowEntities[100000];
    
	pairwise_collision_rule* CollisionRuleHash[256];
	pairwise_collision_rule* FirstFreeCollisionRule;
    
	sim_entity_collision_volume_group* NullCollision;
	sim_entity_collision_volume_group* SwordCollision;
	sim_entity_collision_volume_group* StairCollision;
	sim_entity_collision_volume_group* HeroHeadCollision;
    sim_entity_collision_volume_group* HeroBodyCollision;
	sim_entity_collision_volume_group* MonstarCollision;
	sim_entity_collision_volume_group* WallCollision;
	sim_entity_collision_volume_group* FamiliarCollision;
	sim_entity_collision_volume_group* FloorCollision;
    
	r32 time;
    
    u32 NextParticle;
    particle Particle[256];
    
#define PARTICEL_CEL_DIM 16
    particle_cel ParticleCels[PARTICEL_CEL_DIM][PARTICEL_CEL_DIM];
    
    random_series EffectEntropy;
};

inline low_entity*
GetLowEntity(game_mode_world* World, u32 Index) {
	low_entity* Result = 0;
    
	if ((Index > 0) && (Index < World->LowEntityCount)) {
		Result = World->LowEntities + Index;
	}
	return(Result);
}




#endif //HANDMADE_WORLD_MODE_H
