/* date = January 24th 2021 6:07 pm */

#ifndef HANDMADE_WORLD_MODE_H
#define HANDMADE_WORLD_MODE_H


struct pairwise_collision_rule {
	b32 CanCollide;
	u32 EntityIDA;
	u32 EntityIDB;
    
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
    
    entity_id CameraFollowingEntityID;
	world_position CameraP;
    v3 CameraOffset;
    
	r32 TypicalFloorHeight = 3.0f;
    
	pairwise_collision_rule* CollisionRuleHash[256];
	pairwise_collision_rule* FirstFreeCollisionRule;
    
	entity_collision_volume_group* NullCollision;
	entity_collision_volume_group* StairCollision;
	entity_collision_volume_group* HeroHeadCollision;
    entity_collision_volume_group* HeroBodyCollision;
    entity_collision_volume_group* HeroGloveCollision;
	entity_collision_volume_group* MonstarCollision;
	entity_collision_volume_group* WallCollision;
	entity_collision_volume_group* FamiliarCollision;
	entity_collision_volume_group* FloorCollision;
    
	r32 time;
    
    u32 NextParticle;
    particle Particle[256];
    
    b32 CreationBufferLock;
    u32 CreationBufferIndex;
    entity CreationBuffer[4];
    u32 LastUsedEntityStorageIndex;
    
#define PARTICEL_CEL_DIM 16
    particle_cel ParticleCels[PARTICEL_CEL_DIM][PARTICEL_CEL_DIM];
    
    random_series GameEntropy;
    random_series EffectEntropy;
};


#endif //HANDMADE_WORLD_MODE_H
