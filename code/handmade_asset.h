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
    platform_file_handle Handle;
    hha_header Header;
    hha_asset_type *AssetTypeArray;
    u32 TagBase;
};

struct asset_type {
	uint32 FirstAssetIndex;
	uint32 OnePassLastAssetIndex;
};

enum asset_state {
	AssetState_Unloaded,
	AssetState_Queued,
	AssetState_Loaded,
	
    AssetState_StateMask = 0xFFF,
    AssetState_Locked = 0x10000
};

struct asset_memory_header {
    asset_memory_header *Next;
    asset_memory_header *Prev;
    u32 AssetIndex;
    u32 TotalSize;
    union { 
		loaded_bitmap Bitmap; 
		loaded_sound Sound;
	};
};

struct asset {
    u32 State;
    
    asset_memory_header *Header;
    
    hha_asset HHA;
    u32 FileIndex;
};

struct asset_vector {
	real32 E[Tag_Count];
};

enum asset_memory_block_flag {
    AssetMemory_Used = 0x1,
};

struct asset_memory_block {
    asset_memory_block* Prev;
    asset_memory_block* Next;
    
    // note why 64 for alignment not 32
    u64 Flags;
    memory_index Size;
    
};

struct game_assets {
	struct transient_state* TranState;
    u32 FileCount;
    asset_file *Files;
	
	uint32 AssetCount;
	asset* Assets;
    
    asset_memory_header LoadedAssetSentinel;
    asset_memory_block MemorySentinel;
    
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

inline b32
IsLocked(asset* Asset) {
    b32 Result = Asset->State & AssetState_Locked;
    return(Result);
}

inline u32
GetState(asset* Asset) {
    u32 Result = Asset->State & AssetState_StateMask;
    return(Result);
}

internal void MoveHeaderToFront(game_assets* Assets, asset* Asset);

inline loaded_bitmap*
GetBitmap(game_assets* Assets, bitmap_id ID, b32 MustBelocked) {
	Assert(ID.Value <= Assets->AssetCount);
    
    asset *Asset = Assets->Assets + ID.Value;
    loaded_bitmap* Result = 0;
    if (GetState(Asset) >= AssetState_Loaded) {
        Assert(!MustBelocked || IsLocked(Asset));
        CompletePreviousReadsBeforeFutureReads;
        Result = &Asset->Header->Bitmap;
        MoveHeaderToFront(Assets, Asset);
    }
    
	return(Result);
}

inline loaded_sound*
GetSound(game_assets* Assets, sound_id ID) {
	Assert(ID.Value <= Assets->AssetCount);
    asset *Asset = Assets->Assets + ID.Value;
    loaded_sound* Result = 0;
    if (GetState(Asset) >= AssetState_Loaded) {
        CompletePreviousReadsBeforeFutureReads;
        Result = &Asset->Header->Sound;
        MoveHeaderToFront(Assets, Asset);
    }
	return(Result);
}

internal void LoadBitmap(game_assets* Assets, bitmap_id ID, b32 Locked);