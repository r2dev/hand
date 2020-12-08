#include "handmade_asset.h"

struct load_asset_work {
    task_with_memory* Task;
    asset_slot* Slot;
    platform_file_handle *Handle;
    
    u64 Size;
    u64 Offset;
    void* Destination;
    asset_state FinalState;
    
};

PLATFORM_WORK_QUEUE_CALLBACK(DoLoadAssetWork) {
    load_asset_work * Work = (load_asset_work *)Data;
    _WriteBarrier();
	//Work->Assets->Slots[Work->ID.Value].Bitmap = Bitmap;
	Work->Slot->State = Work->FinalState;
	EndTaskWithMemory(Work->Task);
}

internal
void LoadSound(game_assets* Assets, sound_id ID) {
	if (ID.Value &&
		_InterlockedCompareExchange((long volatile*)&Assets->Slots[ID.Value].State, AssetState_Unloaded, AssetState_Queued) == AssetState_Unloaded) {
		task_with_memory* Task = BeginTaskWithMemory(Assets->TranState);
		if (Task) {
            hha_asset* Asset = Assets->Assets + ID.Value;
            hha_sound* Info = &Asset->Sound;
            
            loaded_sound* Sound = PushStruct(&Assets->Arena, loaded_sound);
            Sound->SampleCount = Info->SampleCount;
            Sound->ChannelCount = Info->ChannelCount;
            u32 ChannelSize = Sound->SampleCount * sizeof(s16);
            u32 MemorySize = ChannelSize * Sound->ChannelCount;
            void *Memory = PushSize(&Assets->Arena, MemorySize);
            s16 *SoundAt = (s16*)Memory;
            
            for (u32 ChannelIndex = 0; ChannelIndex < Sound->ChannelCount; ++ChannelIndex) {
                Sound->Samples[ChannelIndex] = SoundAt;
                SoundAt += ChannelSize;
            }
            
			load_asset_work* Work = PushStruct(&Task->Arena, load_asset_work);
            Work->Slot = Assets->Slots + ID.Value;
            Work->Handle = 0;
            Work->Offset = Asset->DataOffset;
            Work->Size = MemorySize;
			Work->Task = Task;
            Work->Destination = Memory;
            
            Work->Slot->Sound = Sound;
            Copy(Work->Size, Assets->HHAContent + Asset->DataOffset, Work->Destination);
            
			Work->FinalState = AssetState_loaded;
            
			Platform.AddEntry(Assets->TranState->LowPriorityQueue, DoLoadAssetWork, Work);
		}
		else {
			Assets->Slots[ID.Value].State = AssetState_Unloaded;
		}
	}
}

inline void
PrefetchSound(game_assets* Assets, sound_id ID) { LoadSound(Assets, ID); }


internal void
LoadBitmap(game_assets* Assets, bitmap_id ID) {
	if (ID.Value &&
		_InterlockedCompareExchange((long volatile*)&Assets->Slots[ID.Value].State, AssetState_Unloaded, AssetState_Queued) == AssetState_Unloaded) {
		task_with_memory* Task = BeginTaskWithMemory(Assets->TranState);
		if (Task) {
            hha_asset *Asset = Assets->Assets + ID.Value;
            hha_bitmap* Info = &Asset->Bitmap;
            loaded_bitmap* Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
            Bitmap->Width = Info->Dim[0];
            Bitmap->Height = Info->Dim[1];
            Bitmap->Pitch = Bitmap->Width * BITMAP_BYTE_PER_PIXEL;
            Bitmap->WidthOverHeight = SafeRatio1((r32)Bitmap->Width, (r32)Bitmap->Height);
            Bitmap->AlignPercentage = v2{Info->AlignPercentage[0], Info->AlignPercentage[1]};
            u32 MemorySize = Bitmap->Pitch * Bitmap->Height;
            Bitmap->Memory = PushSize(&Assets->Arena, MemorySize);
            
			load_asset_work* Work = PushStruct(&Task->Arena, load_asset_work);
			
            Work->Slot = Assets->Slots + ID.Value;
            Work->Handle = 0;
            Work->Size = MemorySize;
            Work->Offset = Asset->DataOffset;
			Work->Task = Task;
            Work->Destination = Bitmap->Memory;
            Work->Slot->Bitmap = Bitmap;
            Copy(Work->Size, Assets->HHAContent + Asset->DataOffset, Work->Destination);
			Work->FinalState = AssetState_loaded;
            
			Platform.AddEntry(Assets->TranState->LowPriorityQueue, DoLoadAssetWork, Work);
		}
		else {
			Assets->Slots[ID.Value].State = AssetState_Unloaded;
		}
	}
}

internal u32
BestMatchAsset(game_assets* Assets, asset_type_id TypeID, asset_vector* MatchVector, asset_vector* WeightVector) {
	uint32 Result = 0;
	asset_type* Type = Assets->AssetTypes + TypeID;
    if (TypeID == Asset_Head) {
        int a = 0;
    }
	real32 BestDiff = Real32Maximum;
	for (uint32 AssetIndex = Type->FirstAssetIndex; AssetIndex < Type->OnePassLastAssetIndex; ++AssetIndex) {
		hha_asset* Asset = Assets->Assets + AssetIndex;
		real32 TotalWeight = 0.0f;
		for (uint32 TagIndex = Asset->FirstTagIndex; TagIndex < Asset->OnePassLastTagIndex; ++TagIndex) {
			hha_tag* Tag = Assets->Tags + TagIndex;
			real32 A = MatchVector->E[Tag->ID];
			real32 B = Tag->Value;
			real32 D0 = AbsoluteValue(A - B);
			real32 D1 = AbsoluteValue(A - Assets->TagRange[Tag->ID] * SignOf(A) - B);
			TotalWeight += Minimum(D0, D1) * WeightVector->E[Tag->ID];
		}
        
		if (BestDiff > TotalWeight) {
			BestDiff = TotalWeight;
			Result = AssetIndex;
		}
	}
	return(Result);
}

internal bitmap_id
GetBestMatchBitmapFrom(game_assets* Assets, asset_type_id ID, asset_vector* MatchVector, asset_vector* WeightVector) {
	bitmap_id Result = { BestMatchAsset(Assets, ID, MatchVector, WeightVector) };
	return(Result);
}

internal sound_id
GetBestMatchSoundFrom(game_assets* Assets, asset_type_id ID, asset_vector* MatchVector, asset_vector* WeightVector) {
	sound_id Result = { BestMatchAsset(Assets, ID, MatchVector, WeightVector) };
	return(Result);
}

internal game_assets*
AllocateGameAssets(memory_arena* Arena, memory_index Size, transient_state* TranState) {
	game_assets* Assets = PushStruct(Arena, game_assets);
    
	SubArena(&Assets->Arena, Arena, Size);
	Assets->TranState = TranState;
    
    
    for (uint32 Tag = 0; Tag < Tag_Count; ++Tag) {
		Assets->TagRange[Tag] = 100000.0f;
	}
	Assets->TagRange[Tag_FaceDirection] = Tau32;
    Assets->TagCount = 0;
    Assets->AssetCount = 0;
    
#if 0
    platform_file_group* FileGroup = Platform.GetAllFilesOfTypeBegin("hha");
    Assets->FileCount = FileGroup->FileCount;
    Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
    for (u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex) {
        asset_file* File = Assets->Files + FileIndex;
        File->TagBase = Assets->TagCount;
        File->Handle = PlatformOpenFile(FileGroup, FileIndex);
        ZeroStruct(File->Header);
        Platform.ReadDataFromFile(File->Handle, 0, sizeof(File->Header), &File->Header);
        
        u32 AssetTypeArraySize = File->Header.AssetTypeCount * sizeof(hha_asset_type);
        File->AssetTypeArray = (hha_asset_type*)PushSize(Arena, AssetTypeArraySize);
        Platform.ReadDataFromFile(File->Handle, File->Header.AssetTypes, AssetTypeArraySize, File->AssetTypeArray);
        if (File->Header.MagicValue != HHA_MAGIC_VALUE) {
            Platform.FileError(File->Handle, "invalid magic value");
        }
        if (File->Header.Version > HHA_VERSION) {
            Platform.FileError(File->Handle, "for a late version");
        }
        
        
        if (PlatformNoFileErrors(File->Handle)) {
            Assets->TagCount += Header->TagCount;
            Assets->AssetCount += Header->AssetCount;
        } else {
            InvalidCodePath;
        }
    }
    Platform.GetAllFilesOfTypeEnd(FileGroup);
    
    
    Assets->Assets = PushArray(Arena, Assets->AssetCount, hha_asset);
    Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
    Assets->Tag = PushArray(Arena, Assets->TagCount, hha_tag);
    
    for (u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex) {
        asset_file* File = Assets->Files + FileIndex;
        if (PlatformNoFileErrors(File->Handle)) {
            u32 TagArraySize = sizeof(hha_tag) * File->Header.TagCount;
            Platform.ReadDataFromFile(File->Handle, File->Header.Tags, TagArraySize, Assets->Tags + File->TagBase);
        }
    }
    
    u32 AssetCount = 0;
    
    for (u32 DestTypeId = 0; DestTypeId < Asset_Count; ++DestTypeId) {
        asset_type* DestType = Assets->AssetTypes + DestTypeId;
        // todo
        DestType->FirstAssetIndex = AssetCount;
        for (u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex) {
            asset_file *File = Assets->Files + FileIndex;
            if (PlatformNoFileErrors(File->Handle)) {
                for (u32 SourceIndex = 0; SourceIndex < File->Header.AssetTypeCount; ++SourceIndex) {
                    hha_asset_type *SourceType = File->AssetTypeArray + SourceIndex;
                    if (SourceTyppe->TypeId == DestTypeId) {
                        u32 AssetCountForType = SourceType->OnePassLastAssetIndex - SourceType->FirstAssetIndex;
                        Platform.ReadDataFromFile(File->Handle, 
                                                  File->Header.Assets + SourceType->FirstAssetIndex * sizeof(hha_asset),
                                                  AssetCountForType * sizeof(hha_asset),
                                                  Assets->Assets + AssetCount);
                        for (u32 AssetIndex = AssetCount; AssetIndex < AssetCount + AssetCountForType; ++AssetIndex) {
                            hha_asset* Asset = Assets->Assets + AssetIndex;
                            Asset->FirstTagIndex += File->TagBase;
                            Assst->OnePassLastAssetIndex += File->TagBase;
                        }
                        AssetCount += AssetCountForType;
                    }
                }
            }
            
        }
        DestType->OnePassLastAssetIndex = AssetCount;
    }
    
#else
    
    debug_read_file_result Result =  Platform.DEBUGReadEntireFile("test.hha");
    if (Result.ContentsSize) {
        Assets->HHAContent = (u8*)Result.Contents;
        
        hha_header * Header = (hha_header*)Result.Contents;
        
        Assets->AssetCount =  Header->AssetCount;
        Assets->Assets = (hha_asset*)((u8*)Result.Contents + Header->Assets);
        Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
        
        Assets->TagCount = Header->TagCount;
        Assets->Tags = (hha_tag*) ((u8*)Result.Contents + Header->Tags);
        
        hha_asset_type* HHATypes = (hha_asset_type*)((u8*)Result.Contents + Header->AssetTypes);
        
        for (u32 TypeIndex = 0; TypeIndex < Header->AssetTypeCount; ++TypeIndex) {
            hha_asset_type *Source = HHATypes + TypeIndex;
            if (Source->TypeID < Asset_Count) {
                asset_type *Dest = Assets->AssetTypes + Source->TypeID;
                Assert(Dest->FirstAssetIndex == 0);
                Assert(Dest->OnePassLastAssetIndex == 0);
                
                Dest->FirstAssetIndex = Source->FirstAssetIndex;
                Dest->OnePassLastAssetIndex = Source->OnePassLastAssetIndex;
            }
        }
    }
    
#endif
	return(Assets);
}

internal uint32
GetFirstSlotFrom(game_assets *Assets, asset_type_id ID) {
	uint32 Result = 0;
	asset_type* Type = Assets->AssetTypes + ID;
	if (Type->FirstAssetIndex < Type->OnePassLastAssetIndex) {
		Result = Type->FirstAssetIndex;
	}
	return(Result);
}

internal bitmap_id
GetFirstBitmapFrom(game_assets* Asset, asset_type_id ID) {
	bitmap_id Result = { GetFirstSlotFrom(Asset, ID) };
	return(Result);
}

internal sound_id
GetFirstSoundFrom(game_assets* Asset, asset_type_id ID) {
	sound_id Result = { GetFirstSlotFrom(Asset, ID) };
	return(Result);
}

internal uint32
RandomAssetFrom(game_assets* Assets, asset_type_id ID, random_series* Series) {
	uint32 Result = 0;
	asset_type* Type = Assets->AssetTypes + ID;
	if (Type->FirstAssetIndex != Type->OnePassLastAssetIndex) {
		uint32 Count = Type->OnePassLastAssetIndex - Type->FirstAssetIndex;
		Result = Type->FirstAssetIndex + RandomChoice(Series, Count);
	}
	return(Result);
}

internal bitmap_id
GetRandomBitmapFrom(game_assets* Assets, asset_type_id ID, random_series* Series) {
	bitmap_id Result = {RandomAssetFrom(Assets, ID, Series)};
	return(Result);
}