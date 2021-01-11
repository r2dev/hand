#pragma once
struct tile_chunk_position {
	s32 TileChunkX;
	s32 TileChunkY;
	s32 TileChunkZ;
    
	s32 RelTileX;
	s32 RelTileY;
};

struct world_entity_block {
	u32 EntityCount;
	u32 LowEntityIndex[16];
	world_entity_block* Next;
};

struct world_chunk {
	s32 ChunkX;
	s32 ChunkY;
	s32 ChunkZ;
    
	world_entity_block FirstBlock;
    
	world_chunk* NextInHash;
};

struct world {
    
	v3 ChunkDimInMeters;
    
	world_chunk ChunkHash[4096];
    
	world_entity_block* FirstFree;
};


Introspect(category: "world") struct world_position {
	s32 ChunkX;
	s32 ChunkY;
	s32 ChunkZ;
    
	v3 Offset_;
};