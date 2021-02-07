#pragma once
struct tile_chunk_position {
	s32 TileChunkX;
	s32 TileChunkY;
	s32 TileChunkZ;
    
	s32 RelTileX;
	s32 RelTileY;
};

struct world_position {
	s32 ChunkX;
	s32 ChunkY;
	s32 ChunkZ;
    
	v3 Offset_;
};

struct world_entity_block {
	u32 EntityCount;
	u32 LowEntityIndex[16];
	world_entity_block* Next;
    
    u32 EntityDataSize;
    u8 EntityData[1 << 16];
};

struct world_chunk {
	s32 ChunkX;
	s32 ChunkY;
	s32 ChunkZ;
    
	world_entity_block *FirstBlock;
    
	world_chunk* NextInHash;
};


struct world {
	v3 ChunkDimInMeters;
    
	world_chunk *ChunkHash[4096];
    
	world_entity_block* FirstFree;
    
    memory_arena Arena;
    
    world_chunk *FirstFreeChunk;
    world_entity_block *FirstFreeBlock;
};