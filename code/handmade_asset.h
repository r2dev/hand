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
    s32 FontBitmapIDOffset;
};

struct asset_type {
	uint32 FirstAssetIndex;
	uint32 OnePassLastAssetIndex;
};

enum asset_state {
	AssetState_Unloaded,
	AssetState_Queued,
	AssetState_Loaded,
};

struct loaded_font {
    bitmap_id *CodePoints;
    r32 *HorizontalAdvance;
    
    u32 BitmapIDOffset;
};

struct asset_memory_header {
    asset_memory_header *Next;
    asset_memory_header *Prev;
    u32 AssetIndex;
    u32 TotalSize;
    u32 GenerationID;
    union { 
		loaded_bitmap Bitmap; 
		loaded_sound Sound;
        loaded_font Font;
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
    
	memory_arena Arena;
    
    u32 OperationLock;
    
    u32 InFlightGenerationCount;
    u32 InFlightGenerations[16];
    
    u32 NextGenerationID;
    
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


inline void
BeginAssetLock(game_assets *Assets) {
    for(;;) {
        if (AtomicCompareExchangeUInt32(&Assets->OperationLock, 1, 0) == 0) {
            break;
        }
    }
}

inline void
EndAssetLock(game_assets *Assets) {
    CompletePreviousWritesBeforeFutureWrites;
    Assets->OperationLock = 0;
}

inline void
InsertAssetHeaderAtFront(game_assets* Assets, asset_memory_header *Header) {
    asset_memory_header *Sentinel = &Assets->LoadedAssetSentinel;
    
    Header->Prev = Sentinel;
    Header->Next = Sentinel->Next;
    
    Header->Next->Prev = Header;
    Header->Prev->Next = Header;
}

inline void
RemoveAssetHeaderFromList(asset_memory_header* Header) {
    Header->Prev->Next = Header->Next;
    Header->Next->Prev = Header->Prev;
    Header->Next = Header->Prev = 0;
}

internal void MoveHeaderToFront(game_assets* Assets, asset* Asset);

inline asset_memory_header*
GetAsset(game_assets* Assets, u32 ID, u32 GenerationID) {
    Assert(ID <= Assets->AssetCount);
    
    asset *Asset = Assets->Assets + ID;
    asset_memory_header* Result = 0;
    
    BeginAssetLock(Assets);
    if (Asset->State == AssetState_Loaded) {
        Result = Asset->Header;
        RemoveAssetHeaderFromList(Result);
        InsertAssetHeaderAtFront(Assets, Result);
        if (Asset->Header->GenerationID < GenerationID) {
            Asset->Header->GenerationID = GenerationID;
        }
        CompletePreviousWritesBeforeFutureWrites;
    }
    EndAssetLock(Assets);
    
	return(Result);
    
}

inline loaded_bitmap*
GetBitmap(game_assets* Assets, bitmap_id ID, u32 GenerationID) {
    asset_memory_header* Header = GetAsset(Assets, ID.Value, GenerationID);
    loaded_bitmap* Result = 0;
    if(Header) {
        Result = &Header->Bitmap;
    }
    return(Result);
}

inline loaded_sound*
GetSound(game_assets* Assets, sound_id ID, u32 GenerationID) {
	asset_memory_header* Header = GetAsset(Assets, ID.Value, GenerationID);
    loaded_sound* Result = 0;
    if (Header) {
        Result = &Header->Sound;
    }
    return(Result);
}


inline loaded_font*
GetFont(game_assets* Assets, font_id ID, u32 GenerationID) {
	asset_memory_header* Header = GetAsset(Assets, ID.Value, GenerationID);
    loaded_font* Result = 0;
    if (Header) {
        Result = &Header->Font;
    }
    
    return(Result);
}


inline u32
BeginGeneration(game_assets* Assets) {
    BeginAssetLock(Assets);
    Assert(Assets->InFlightGenerationCount < ArrayCount(Assets->InFlightGenerations));
    u32 Result = Assets->NextGenerationID++;
    Assets->InFlightGenerations[Assets->InFlightGenerationCount++] = Result;
    EndAssetLock(Assets);
    return(Result);
}

inline void
EndGeneration(game_assets* Assets, u32 GenerationID) {
    BeginAssetLock(Assets);
    for (u32 Index = 0; Index < Assets->InFlightGenerationCount; ++Index) {
        if (Assets->InFlightGenerations[Index] == GenerationID) {
            Assets->InFlightGenerations[Index] = Assets->InFlightGenerations[--Assets->InFlightGenerationCount];
            break;
        }
    }
    
    EndAssetLock(Assets);
}

internal void LoadBitmap(game_assets* Assets, bitmap_id ID, b32 Immediate);