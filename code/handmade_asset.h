#pragma once
struct hero_bitmaps {
	loaded_bitmap Head;
	loaded_bitmap Cape;
	loaded_bitmap Torso;
};

struct bitmap_id {
	uint32 Value;
};

struct audio_id {
	uint32 Value;
};

enum asset_type_id {
	Asset_none,
	Asset_Shadow,
	Asset_Tree,
	Asset_Sword,
	Asset_Count
};

enum asset_tag_id {
	Tag_Smoothness,
	Tag_Flagness,
	Tag_Count
};

struct asset_tag {
	uint32 ID;
	real32 Value;
};

struct asset {
	uint32 FirstTagIndex;
	uint32 OnePassLastTagIndex;
	uint32 SlotID;
};

struct asset_type {
	uint32 FirstAssetIndex;
	uint32 OnePassLastAssetIndex;
};


enum asset_state {
	AssetState_Unloaded,
	AssetState_Queued,
	AssetState_loaded,
	AssetState_locked,
};

struct asset_slot {
	loaded_bitmap* Bitmap;
	asset_state State;
};
struct game_assets {
	struct transient_state* TranState;
	uint32_t BitmapsCount;
	asset_slot *Bitmaps;

	uint32_t TagCount;
	asset_tag* Tags;

	uint32_t AssetCount;
	asset* Assets;

	asset_type AssetTypes[Asset_Count];

	debug_platform_read_entire_file* ReadEntireFile;
	memory_arena Arena;
};

inline loaded_bitmap*
GetBitmap(game_assets* Assets, bitmap_id ID) {
	loaded_bitmap* Result = Assets->Bitmaps[ID.Value].Bitmap;
	return(Result);
}

internal void LoadBitmap(game_assets* Assets, bitmap_id ID);