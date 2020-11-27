#pragma once

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

	Asset_Grass,
	Asset_Ground,
	Asset_Tuft,

	Asset_Head,
	Asset_Cape,
	Asset_Torso,

	Asset_Count
};

enum asset_tag_id {
	Tag_Smoothness,
	Tag_Flagness,
	Tag_FaceDirection,


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

struct asset_bitmap_info {
	char* FileName;
	v2 AlignPercentage;
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

struct asset_vector {
	real32 E[Tag_Count];
};

struct game_assets {
	struct transient_state* TranState;
	uint32 BitmapsCount;
	asset_bitmap_info* BitmapInfos;
	asset_slot *Bitmaps;

	uint32 TagCount;
	asset_tag* Tags;

	uint32 AssetCount;
	asset* Assets;

	asset_type AssetTypes[Asset_Count];

	memory_arena Arena;

	uint32 DEBUGUsedBitmapCount;
	uint32 DEBUGUsedAssetCount;
	uint32 DEBUGUsedTagCount;
	asset_type* DEBUGAssetType;
	asset* DEBUGAsset;

};

inline loaded_bitmap*
GetBitmap(game_assets* Assets, bitmap_id ID) {
	loaded_bitmap* Result = Assets->Bitmaps[ID.Value].Bitmap;
	return(Result);
}

internal void LoadBitmap(game_assets* Assets, bitmap_id ID);