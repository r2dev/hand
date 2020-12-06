#include "handmade_asset.h"

struct load_sound_work {
	loaded_sound* Sound;
	sound_id ID;
	game_assets* Assets;
	task_with_memory* Task;
	asset_state FinalState;
};

loaded_sound
DEBUGLoadWAV(char* FileName, u32 FirstSampleIndex, u32 SampleCount) {
    Assert(!"No");
    loaded_sound Sound = {};
    return(Sound);
}

loaded_bitmap
DEBUGLoadBMP(char* FileName, v2 AlignPercentage) {
    Assert(!"No");
    loaded_bitmap Bitmap = {};
    return(Bitmap);
}


PLATFORM_WORK_QUEUE_CALLBACK(DoLoadSoundWork) {
	load_sound_work* Work = (load_sound_work*)Data;
	asset_sound_info* Info = &Work->Assets->Assets[Work->ID.Value].Sound;
	*Work->Sound = DEBUGLoadWAV(Info->FileName, Info->FirstSampleIndex, Info->SampleCount);
	_WriteBarrier();
	Work->Assets->Slots[Work->ID.Value].Sound = Work->Sound;
	Work->Assets->Slots[Work->ID.Value].State = Work->FinalState;
	EndTaskWithMemory(Work->Task);
}

internal
void LoadSound(game_assets* Assets, sound_id ID) {
	if (ID.Value &&
		_InterlockedCompareExchange((long volatile*)&Assets->Slots[ID.Value].State, AssetState_Unloaded, AssetState_Queued) == AssetState_Unloaded) {
		task_with_memory* Task = BeginTaskWithMemory(Assets->TranState);
		if (Task) {
			load_sound_work* Work = PushStruct(&Task->Arena, load_sound_work);
			Work->Assets = Assets;
			Work->ID = ID;
			Work->Task = Task;
            
			Work->Sound = PushStruct(&Assets->Arena, loaded_sound);
			Work->FinalState = AssetState_loaded;
            
			PlatformAddEntry(Assets->TranState->LowPriorityQueue, DoLoadSoundWork, Work);
		}
		else {
			Assets->Slots[ID.Value].State = AssetState_Unloaded;
		}
	}
}

inline void
PrefetchSound(game_assets* Assets, sound_id ID) { LoadSound(Assets, ID); }


struct load_bitmap_work {
	loaded_bitmap* Bitmap;
	bitmap_id ID;
	game_assets* Assets;
	task_with_memory* Task;
	asset_state FinalState;
};

PLATFORM_WORK_QUEUE_CALLBACK(DoLoadBitmapWork) {
	load_bitmap_work* Work = (load_bitmap_work*)Data;
	asset_bitmap_info* Info = &Work->Assets->Assets[Work->ID.Value].Bitmap;
	*Work->Bitmap = DEBUGLoadBMP(Info->FileName, Info->AlignPercentage);
	_WriteBarrier();
	Work->Assets->Slots[Work->ID.Value].Bitmap = Work->Bitmap;
	Work->Assets->Slots[Work->ID.Value].State = Work->FinalState;
	EndTaskWithMemory(Work->Task);
}


internal void
LoadBitmap(game_assets* Assets, bitmap_id ID) {
	if (ID.Value &&
		_InterlockedCompareExchange((long volatile*)&Assets->Slots[ID.Value].State, AssetState_Unloaded, AssetState_Queued) == AssetState_Unloaded) {
		task_with_memory* Task = BeginTaskWithMemory(Assets->TranState);
		if (Task) {
			load_bitmap_work* Work = PushStruct(&Task->Arena, load_bitmap_work);
            
			Work->Assets = Assets;
			Work->ID = ID;
			Work->Task = Task;
			
			Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
			Work->FinalState = AssetState_loaded;
            
			PlatformAddEntry(Assets->TranState->LowPriorityQueue, DoLoadBitmapWork, Work);
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
	real32 BestDiff = Real32Maximum;
	for (uint32 AssetIndex = Type->FirstAssetIndex; AssetIndex < Type->OnePassLastAssetIndex; ++AssetIndex) {
		asset* Asset = Assets->Assets + AssetIndex;
		real32 TotalWeight = 0.0f;
		for (uint32 TagIndex = Asset->FirstTagIndex; TagIndex < Asset->OnePassLastTagIndex; ++TagIndex) {
			asset_tag* Tag = Assets->Tags + TagIndex;
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
    
	Assets->TagCount = 1024 * Asset_Count;
	Assets->Tags = PushArray(Arena, Assets->TagCount, asset_tag);
    
	Assets->AssetCount = 2 * 255 * Asset_Count;
	Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);
	Assets->Slots = PushArray(Arena, Assets->AssetCount, asset_slot);
#if 0
	Assets->DEBUGUsedAssetCount = 1;
	// @note(ren) maybe we dont need skip the first tag
	Assets->DEBUGUsedTagCount = 0;
    
	Assets->DEBUGAsset = 0;
	Assets->DEBUGAssetType = 0;
    
	for (uint32 Tag = 0; Tag < Tag_Count; ++Tag) {
		Assets->TagRange[Tag] = 100000.0f;
	}
	Assets->TagRange[Tag_FaceDirection] = Tau32;
	
	BeginAssetType(Assets, Asset_Shadow);
	AddBitmapAsset(Assets, "test/test_hero_shadow.bmp", V2(0.5f, 0.156682029f));
	EndAssetType(Assets);
    
	BeginAssetType(Assets, Asset_Tree);
	AddBitmapAsset(Assets, "test2/tree00.bmp", V2(0.493827164f, 0.295652181f));
	EndAssetType(Assets);
    
	BeginAssetType(Assets, Asset_Sword);
	AddBitmapAsset(Assets, "test2/rock02.bmp", V2(0.5f, 0.65625f));
	EndAssetType(Assets);
    
    
	BeginAssetType(Assets, Asset_Grass);
	AddBitmapAsset(Assets, "test2/grass00.bmp");
	AddBitmapAsset(Assets, "test2/grass01.bmp");
	EndAssetType(Assets);
    
	BeginAssetType(Assets, Asset_Ground);
	AddBitmapAsset(Assets, "test2/ground00.bmp");
	AddBitmapAsset(Assets, "test2/ground01.bmp");
	AddBitmapAsset(Assets, "test2/ground02.bmp");
	AddBitmapAsset(Assets, "test2/ground03.bmp");
	EndAssetType(Assets);
    
    
	BeginAssetType(Assets, Asset_Tuft);
	AddBitmapAsset(Assets, "test2/tuft00.bmp");
	AddBitmapAsset(Assets, "test2/tuft01.bmp");
	EndAssetType(Assets);
    
	real32 AngleRight = 0.0f * Tau32;
	real32 AngleBack = 0.25f * Tau32;
	real32 AngleLeft = 0.5f * Tau32;
	real32 AngleFront = 0.75f * Tau32;
    
	v2 HeroAlign = { 0.5f, 0.156682029f };
    
	BeginAssetType(Assets, Asset_Head);
	AddBitmapAsset(Assets, "test/test_hero_right_head.bmp", HeroAlign);
	AddTag(Assets, Tag_FaceDirection, AngleRight);
	AddBitmapAsset(Assets, "test/test_hero_back_head.bmp", HeroAlign);
	AddTag(Assets, Tag_FaceDirection, AngleBack);
	AddBitmapAsset(Assets, "test/test_hero_left_head.bmp", HeroAlign);
	AddTag(Assets, Tag_FaceDirection, AngleLeft);
	AddBitmapAsset(Assets, "test/test_hero_front_head.bmp", HeroAlign);
	AddTag(Assets, Tag_FaceDirection, AngleFront);
	EndAssetType(Assets);
    
	BeginAssetType(Assets, Asset_Cape);
	AddBitmapAsset(Assets, "test/test_hero_right_cape.bmp", HeroAlign);
	AddTag(Assets, Tag_FaceDirection, AngleRight);
	AddBitmapAsset(Assets, "test/test_hero_back_cape.bmp", HeroAlign);
	AddTag(Assets, Tag_FaceDirection, AngleBack);
	AddBitmapAsset(Assets, "test/test_hero_left_cape.bmp", HeroAlign);
	AddTag(Assets, Tag_FaceDirection, AngleLeft);
	AddBitmapAsset(Assets, "test/test_hero_front_cape.bmp", HeroAlign);
	AddTag(Assets, Tag_FaceDirection, AngleFront);
	EndAssetType(Assets);
    
	BeginAssetType(Assets, Asset_Torso);
	AddBitmapAsset(Assets, "test/test_hero_right_torso.bmp", HeroAlign);
	AddTag(Assets, Tag_FaceDirection, AngleRight);
	AddBitmapAsset(Assets, "test/test_hero_back_torso.bmp", HeroAlign);
	AddTag(Assets, Tag_FaceDirection, AngleBack);
	AddBitmapAsset(Assets, "test/test_hero_left_torso.bmp", HeroAlign);
	AddTag(Assets, Tag_FaceDirection, AngleLeft);
	AddBitmapAsset(Assets, "test/test_hero_front_torso.bmp", HeroAlign);
	AddTag(Assets, Tag_FaceDirection, AngleFront);
	EndAssetType(Assets);
    
    
	BeginAssetType(Assets, Asset_Music);
	u32 TotalSampleCount = 7468095;
	u32 MusicChunkSize = 10 * 48000;
	sound_id LastMusic = {};
    
	for (u32 FirstSampleIndex = 0; FirstSampleIndex < TotalSampleCount; FirstSampleIndex += MusicChunkSize) {
		u32 SampleCount = TotalSampleCount - FirstSampleIndex;
		if (SampleCount > MusicChunkSize) {
			SampleCount = MusicChunkSize;
		}
		sound_id thisMusic = AddSoundAsset(Assets, "test3/music_test.wav", FirstSampleIndex, SampleCount);
		if (IsValid(LastMusic)) {
			Assets->Assets[LastMusic.Value].Sound.NextIDToPlay = thisMusic;
		}
		LastMusic = thisMusic;
	}
	
	EndAssetType(Assets);
    
	BeginAssetType(Assets, Asset_Bloop);
	AddSoundAsset(Assets, "test3/bloop_00.wav");
	AddSoundAsset(Assets, "test3/bloop_01.wav");
	AddSoundAsset(Assets, "test3/bloop_02.wav");
	AddSoundAsset(Assets, "test3/bloop_03.wav");
	AddSoundAsset(Assets, "test3/bloop_04.wav");
	EndAssetType(Assets);
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