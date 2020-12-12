#pragma once

struct loaded_bitmap {
    void* Memory;
	v2 AlignPercentage;
	r32 WidthOverHeight;
    
	s16 Width;
	s16 Height;
	s16 Pitch;
};
struct environment_map {
	loaded_bitmap LOD[4];
	real32 Pz;
};

struct loaded_sound {
	u32 SampleCount;
	u32 ChannelCount;
	s16* Samples[2];
};

struct asset_file {
    platform_file_handle *Handle;
    hha_header Header;
    hha_asset_type *AssetTypeArray;
    u32 TagBase;
};

struct asset_memory_header {
    asset_memory_header *Next;
    asset_memory_header *Prev;
    u32 SlotIndex;
    u32 Reserved;
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
    
    
    AssetState_StateMask = 0xFFF,
    AssetState_Sound = 0x1000,
    AssetState_Bitmap = 0x2000,
    AssetState_TypeMask = 0xF000
};

struct asset_slot {
    u32 State;
	union { 
		loaded_bitmap Bitmap; 
		loaded_sound Sound;
	};
};

struct asset {
    hha_asset HHA;
    u32 FileIndex;
};

struct asset_vector {
	real32 E[Tag_Count];
};

struct game_assets {
	struct transient_state* TranState;
    u32 FileCount;
    
    u64 TotalMemoryUsed;
    u64 TargetMemoryUsed;
    asset_file *Files;
	
	uint32 AssetCount;
	asset* Assets; 
	asset_slot* Slots;
    
    asset_memory_header LoadedAssetSentinel;
    
	real32 TagRange[Tag_Count];
	uint32 TagCount;
	hha_tag* Tags;
    
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

inline u32
GetState(asset_slot* Slot) {
    u32 Result = Slot->State & AssetState_StateMask;
    return(Result);
}

inline u32
GetType(asset_slot* Slot) {
    u32 Result = Slot->State & AssetState_TypeMask;
    return(Result);
}

inline loaded_bitmap*
GetBitmap(game_assets* Assets, bitmap_id ID) {
	Assert(ID.Value <= Assets->AssetCount);
    
    asset_slot *Slot = Assets->Slots + ID.Value;
    loaded_bitmap* Result = 0;
    if (GetState(Slot) >= AssetState_loaded) {
        CompletePreviousReadsBeforeFutureReads;
        Result = &Slot->Bitmap;
    }
    
	return(Result);
}

inline loaded_sound*
GetSound(game_assets* Assets, sound_id ID) {
	Assert(ID.Value <= Assets->AssetCount);
    asset_slot *Slot = Assets->Slots + ID.Value;
    loaded_sound* Result = 0;
    if (GetState(Slot) >= AssetState_loaded) {
        CompletePreviousReadsBeforeFutureReads;
        Result = &Slot->Sound;
    }
	return(Result);
}

internal void LoadBitmap(game_assets* Assets, bitmap_id ID);