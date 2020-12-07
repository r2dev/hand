#pragma once

struct loaded_bitmap {
	v2 AlignPercentage;
	real32 WidthOverHeight;
    
	int32 Width;
	int32 Height;
	int32 Pitch;
	void* Memory;
};
struct environment_map {
	loaded_bitmap LOD[4];
	real32 Pz;
};

struct loaded_sound {
	uint32 SampleCount;
	uint32 ChannelCount;
	int16* Samples[2];
};

struct asset_tag {
	uint32 ID;
	real32 Value;
};


struct asset {
	hha_asset HHAAsset;
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
	asset_state State;
	union { 
		loaded_bitmap* Bitmap; 
		loaded_sound* Sound;
	};
};

struct asset_vector {
	real32 E[Tag_Count];
};

struct game_assets {
	struct transient_state* TranState;
    
	
	uint32 AssetCount;
	asset* Assets; 
	asset_slot* Slots;
    
	real32 TagRange[Tag_Count];
	uint32 TagCount;
	asset_tag* Tags;
    
	asset_type AssetTypes[Asset_Count];
    
    u8* HHAContent;
    
	memory_arena Arena;
    
#if 0
	uint32 DEBUGUsedAssetCount;
	uint32 DEBUGUsedTagCount;
	asset_type* DEBUGAssetType;
	asset* DEBUGAsset;
#endif
    
};

inline b32
IsValid(sound_id ID) {
	b32 Result = ID.Value != 0;
	return(Result);
}
inline b32
IsValid(bitmap_id ID) {
	b32 Result = ID.Value != 0;
	return(Result);
}

inline loaded_bitmap*
GetBitmap(game_assets* Assets, bitmap_id ID) {
	Assert(ID.Value < Assets->AssetCount);
	loaded_bitmap* Result = Assets->Slots[ID.Value].Bitmap;
	return(Result);
}

inline loaded_sound*
GetSound(game_assets* Assets, sound_id ID) {
	Assert(ID.Value < Assets->AssetCount);
	loaded_sound* Result = 0;
    if (Assets->Slots[ID.Value].State == AssetState_loaded) {
        Result = Assets->Slots[ID.Value].Sound;
    }
	return(Result);
}


internal void LoadBitmap(game_assets* Assets, bitmap_id ID);