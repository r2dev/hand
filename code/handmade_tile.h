#pragma once
struct tile_chunk_position {
	int32 TileChunkX;
	int32 TileChunkY;
	int32 TileChunkZ;

	int32 RelTileX;
	int32 RelTileY;
};

struct tile_chunk {
	uint32* Tiles;

	int32 TileChunkX;
	int32 TileChunkY;
	int32 TileChunkZ;

	tile_chunk* NextInHash;
};

struct tile_map {
	int32 ChunkShift;
	int32 ChunkMask;
	int32 ChunkDim;

	real32 TileSideInMeters;

	tile_chunk TileChunkHash[4096];
};

struct tile_map_difference {
	v2 dXY;
	real32 dZ;
};

struct tile_map_position {
	int32 AbsTileX;
	int32 AbsTileY;
	int32 AbsTileZ;

	v2 Offset_;
};