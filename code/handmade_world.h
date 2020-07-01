#pragma once
struct tile_chunk_position {
	int32 TileChunkX;
	int32 TileChunkY;
	int32 TileChunkZ;

	int32 RelTileX;
	int32 RelTileY;
};

struct world_entity_block {
	uint32 EntityCount;
	uint32 LowEntityIndex[16];
	world_entity_block* Next;
};

struct world_chunk {
	int32 ChunkX;
	int32 ChunkY;
	int32 ChunkZ;

	world_entity_block FirstBlock;

	world_chunk* NextInHash;
};

struct world {
	real32 TileSideInMeters;
	real32 TileDepthInMeters;
	v3 ChunkDimInMeters;

	world_chunk ChunkHash[4096];

	world_entity_block* FirstFree;
};


struct world_position {
	int32 ChunkX;
	int32 ChunkY;
	int32 ChunkZ;

	v3 Offset_;
};