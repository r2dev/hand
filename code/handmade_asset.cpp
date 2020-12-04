#include "handmade_asset.h"
#include "handmade.h"
#pragma pack(push, 1)
struct bitmap_header {
	uint16 FileType;
	uint32 FileSize;
	uint16 Reserved1;
	uint16 Reserved2;
	uint32 BitmapOffset;
	uint32 Size;
	int32 Width;
	int32 Height;
	uint16 Planes;
	uint16 BitsPerPixel;
	uint32 Compression;
	uint32 SizeOfBitmap;
	int32 HorzResolution;
	int32 VertResolution;
	uint32 ColorsUsed;
	uint32 ColorsImportant;
	uint32 RedMask;
	uint32 GreenMask;
	uint32 BlueMask;
};


struct WAVE_header {
	uint32 RIFFID;
	uint32 Size;
	uint32 WaveID;
};

#define RIFF_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))
enum {
	WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
	WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
	WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
	WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
};	

struct WAVE_chunk {
	uint32 ID;
	uint32 Size;
};

struct WAVE_fmt {
	uint16 wFormatTag;
	uint16 nChannels;
	uint32 nSamplesPerSec;
	uint32 nAvgBytesPerSec;
	uint16 nBlockAlign;
	uint16 wBitsPerSample;
	uint16 cbSize;
	uint16 wValidBitsPerSample;
	uint32 dwChannelMask;
	uint8 SubFormat[16];

};

#pragma pack(pop)

struct riff_iterator {
	uint8* At;
	uint8* Stop;
};

inline riff_iterator
ParseChunkAt(void* At, void* Stop) {
	riff_iterator Iter;
	Iter.At = (uint8*)At;
	Iter.Stop = (uint8*)Stop;
	return(Iter);

}

inline bool32
IsValid(riff_iterator Iter) {
	bool32 Result = (Iter.At < Iter.Stop);
	return(Result);
}

inline riff_iterator
NextChunk(riff_iterator Iter) {
	WAVE_chunk* Chunk = (WAVE_chunk*)Iter.At;
	// padding
	uint32 Size = (Chunk->Size + 1) & ~1;
	Iter.At += sizeof(WAVE_chunk) + Size;
	return(Iter);
}

inline uint32
GetType(riff_iterator Iter) {
	WAVE_chunk* Chunk = (WAVE_chunk*)Iter.At;
	uint32 Result = Chunk->ID;
	return(Result);
}



inline void*
GetChunkData(riff_iterator Iter) {
	void* Result = (Iter.At + sizeof(WAVE_chunk));
	return(Result);
}

inline uint32
GetChunkDataSize(riff_iterator Iter) {
	WAVE_chunk* Chunk = (WAVE_chunk*)Iter.At;
	uint32 Result = Chunk->Size;
	return(Result);
}

internal loaded_sound
DEBUGLoadWAV(char* FileName, u32 FirstSampleIndex, u32 LoadSampleCount) {
	loaded_sound Result = {};
	debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(FileName);
	if (ReadResult.ContentsSize != 0) {
		WAVE_header* Header = (WAVE_header*)ReadResult.Contents;
		uint32 ChannelCount = 0;
		uint32 SampleDataSize = 0;
		int16* SampleData = 0;
#if 0
		for (riff_iterator Iter = ParseChunkAt(Header + 1, (uint8*)(Header + 1) + Header->Size - 4);
			IsValid(Iter);
			Iter = NextChunk(Iter)
		) {
#else
		for (riff_iterator Iter = ParseChunkAt(Header + 1, (uint8*)(Header + 1) + Header->Size - 4);
			IsValid(Iter);
			Iter = NextChunk(Iter)
			) {
#endif
			switch (GetType(Iter)) {
			
			case WAVE_ChunkID_data: {
				SampleData = (int16*)GetChunkData(Iter);
				SampleDataSize = GetChunkDataSize(Iter);
			} break;
			case WAVE_ChunkID_fmt: {
				WAVE_fmt* fmt = (WAVE_fmt*)GetChunkData(Iter);
				
				Assert(fmt->wFormatTag == 1);
				Assert(fmt->nSamplesPerSec == 48000);

				ChannelCount = fmt->nChannels;
			} break;
			}
		}
		Assert(SampleData && ChannelCount);
		Result.ChannelCount = ChannelCount;
		u32 SampleCount = SampleDataSize / (ChannelCount * sizeof(int16));

		if (ChannelCount == 1) {
			Result.Samples[0] = SampleData;
			Result.Samples[1] = 0;
		}
		else if (ChannelCount == 2) {
			Result.Samples[0] = SampleData;
			Result.Samples[1] = SampleData + Result.SampleCount;
			for (uint32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex) {
				int16 Source = SampleData[2 * SampleIndex];
				SampleData[2 * SampleIndex] = SampleData[SampleIndex];
				SampleData[SampleIndex] = Source;
			}
		}
		else {
			Assert(!"Invalid Channel Count");
		}

		// todo remove
		Result.ChannelCount = 1;

		b32 AtEnd = true;

		if (LoadSampleCount) {
			Assert(LoadSampleCount + FirstSampleIndex <= SampleCount);
			AtEnd = (LoadSampleCount + FirstSampleIndex) == SampleCount;
			SampleCount = LoadSampleCount;
			for (u32 ChannelIndex = 0; ChannelIndex < Result.ChannelCount; ++ChannelIndex) {
				Result.Samples[ChannelIndex] += FirstSampleIndex;
			}
		}

		if (AtEnd) {
			for (u32 ChannelIndex = 0; ChannelIndex < Result.ChannelCount; ++ChannelIndex) {
				for (u32 SampleIndex = SampleCount; SampleIndex < SampleCount + 8; ++SampleIndex) {
					Result.Samples[ChannelIndex][SampleIndex] = 0;
				}
			}
		}
		Result.SampleCount = SampleCount;
	}
	return(Result);
}

struct load_sound_work {
	loaded_sound* Sound;
	sound_id ID;
	game_assets* Assets;
	task_with_memory* Task;
	asset_state FinalState;
};

PLATFORM_WORK_QUEUE_CALLBACK(DoLoadSoundWork) {
	load_sound_work* Work = (load_sound_work*)Data;
	asset_sound_info* Info = Work->Assets->SoundInfos + Work->ID.Value;
	*Work->Sound = DEBUGLoadWAV(Info->FileName, Info->FirstSampleIndex, Info->SampleCount);
	_WriteBarrier();
	Work->Assets->Sounds[Work->ID.Value].Sound = Work->Sound;
	Work->Assets->Sounds[Work->ID.Value].State = Work->FinalState;
	EndTaskWithMemory(Work->Task);
}

internal
void LoadSound(game_assets* Assets, sound_id ID) {
	if (ID.Value &&
		_InterlockedCompareExchange((long volatile*)&Assets->Sounds[ID.Value].State, AssetState_Unloaded, AssetState_Queued) == AssetState_Unloaded) {
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
			Assets->Sounds[ID.Value].State = AssetState_Unloaded;
		}
	}
}

inline void
PrefetchSound(game_assets* Assets, sound_id ID) { LoadSound(Assets, ID); }

internal loaded_bitmap
DEBUGLoadBMP(char* FileName, v2 AlignPercentage = v2{0.5f, 0.5f}) {
	loaded_bitmap Result = {};
	debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(FileName);
	if (ReadResult.ContentsSize != 0) {
		bitmap_header* Header = (bitmap_header*)ReadResult.Contents;
		uint32* Pixels = (uint32*)((uint8*)ReadResult.Contents + Header->BitmapOffset);
		Result.Height = Header->Height;
		Result.Width = Header->Width;
		Result.AlignPercentage = AlignPercentage;
		Result.Memory = Pixels;
		Result.WidthOverHeight = SafeRatio0((real32)Result.Width, (real32)Result.Height);
		uint32 RedMask = Header->RedMask;
		uint32 GreenMask = Header->GreenMask;
		uint32 BlueMask = Header->BlueMask;
		uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

		bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
		bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
		bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
		bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);
		Assert(RedScan.Found);
		Assert(GreenScan.Found);
		Assert(BlueScan.Found);
		Assert(AlphaScan.Found);

		int32 RedShiftDown = (int32)RedScan.Index;
		int32 GreenShiftDown = (int32)GreenScan.Index;
		int32 BlueShiftDown = (int32)BlueScan.Index;
		int32 AlphaShiftDown = (int32)AlphaScan.Index;

		uint32* SourceDest = Pixels;
		for (int32 Y = 0;
			Y < Header->Height;
			++Y)
		{
			for (int32 X = 0;
				X < Header->Width;
				++X)
			{
				uint32 C = *SourceDest;
				v4 Texel = {
					(real32)((C & RedMask) >> RedShiftDown),
					(real32)((C & GreenMask) >> GreenShiftDown),
					(real32)((C & BlueMask) >> BlueShiftDown),
					(real32)((C & AlphaMask) >> AlphaShiftDown)
				};
				Texel = SRGBToLinear1(Texel);
				Texel.rgb *= Texel.a;
				Texel = Linear1ToSRGB(Texel);

				*SourceDest++ = ((uint32)(Texel.a + 0.5f) << 24) |
					((uint32)(Texel.r + 0.5f) << 16) |
					((uint32)(Texel.g + 0.5f) << 8) |
					((uint32)(Texel.b + 0.5f) << 0);
			}
		}
	}
	Result.Pitch = Result.Width * BITMAP_BYTE_PER_PIXEL;
#if 0
	Result.Pitch = -Result.Width * BITMAP_BYTE_PER_PIXEL;
	Result.Memory = (uint8*)Result.Memory - Result.Pitch * (Result.Height - 1);
#endif

	return(Result);

}

struct load_bitmap_work {
	loaded_bitmap* Bitmap;
	bitmap_id ID;
	game_assets* Assets;
	task_with_memory* Task;
	asset_state FinalState;
};

PLATFORM_WORK_QUEUE_CALLBACK(DoLoadBitmapWork) {
	load_bitmap_work* Work = (load_bitmap_work*)Data;
	asset_bitmap_info* Info = Work->Assets->BitmapInfos + Work->ID.Value;
	*Work->Bitmap = DEBUGLoadBMP(Info->FileName, Info->AlignPercentage);
	_WriteBarrier();
	Work->Assets->Bitmaps[Work->ID.Value].Bitmap = Work->Bitmap;
	Work->Assets->Bitmaps[Work->ID.Value].State = Work->FinalState;
	EndTaskWithMemory(Work->Task);
}


internal void
LoadBitmap(game_assets* Assets, bitmap_id ID) {
	if (ID.Value &&
		_InterlockedCompareExchange((long volatile*)&Assets->Bitmaps[ID.Value].State, AssetState_Unloaded, AssetState_Queued) == AssetState_Unloaded) {
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
			Assets->Bitmaps[ID.Value].State = AssetState_Unloaded;
		}
	}
}
internal bitmap_id
DEBUGAddBitmapInfo(game_assets* Assets, char* FileName, v2 AlignmentPercentage) {
	bitmap_id ID = { Assets->DEBUGUsedBitmapCount++ };
	asset_bitmap_info* Info = Assets->BitmapInfos + ID.Value;
	Info->AlignPercentage = AlignmentPercentage;
	Info->FileName = PushString(&Assets->Arena, FileName);
	return(ID);
}

internal sound_id
DEBUGAddSoundInfo(game_assets* Assets, char* FileName, u32 FirstSampleIndex, u32 LoadSampleCount) {
	sound_id ID = { Assets->DEBUGUsedSoundCount++ };
	asset_sound_info* Info = Assets->SoundInfos + ID.Value;
	Info->FileName = PushString(&Assets->Arena, FileName);
	Info->FirstSampleIndex = FirstSampleIndex;
	Info->SampleCount = LoadSampleCount;
	Info->NextIDToPlay.Value = 0;
	return(ID);
}

internal void
BeginAssetType(game_assets* Assets, asset_type_id ID) {
	Assert(Assets->DEBUGAssetType == 0);
	Assets->DEBUGAssetType = Assets->AssetTypes + ID;
	Assets->DEBUGAssetType->FirstAssetIndex = Assets->DEBUGUsedAssetCount++;
	Assets->DEBUGAssetType->OnePassLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;

}

internal void
AddBitmapAsset(game_assets* Assets, char* FileName, v2 AlignmentPercentage = v2{0.5f, 0.5f}) {
	Assert(Assets->DEBUGAssetType);
	asset* Asset = Assets->Assets + Assets->DEBUGAssetType->OnePassLastAssetIndex++;
	Asset->FirstTagIndex = Assets->DEBUGUsedTagCount;
	Asset->OnePassLastTagIndex = Asset->FirstTagIndex;
	Asset->SlotID = DEBUGAddBitmapInfo(Assets, FileName, AlignmentPercentage).Value;
	Assets->DEBUGAsset = Asset;
}

internal asset*
AddSoundAsset(game_assets* Assets, char* FileName, u32 FirstSampleIndex = 0, u32 LoadSampleCount = 0) {
	Assert(Assets->DEBUGAssetType);
	asset* Asset = Assets->Assets + Assets->DEBUGAssetType->OnePassLastAssetIndex++;
	Asset->FirstTagIndex = Assets->DEBUGUsedTagCount;
	Asset->OnePassLastTagIndex = Asset->FirstTagIndex;
	Asset->SlotID = DEBUGAddSoundInfo(Assets, FileName, FirstSampleIndex, LoadSampleCount).Value;
	Assets->DEBUGAsset = Asset;
	return(Asset);
}


internal void
EndAssetType(game_assets* Assets) {
	Assert(Assets->DEBUGAssetType);
	Assets->DEBUGUsedAssetCount = Assets->DEBUGAssetType->OnePassLastAssetIndex;
	Assets->DEBUGAssetType = 0;
}

internal uint32
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
			Result = Asset->SlotID;
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

internal void
AddTag(game_assets* Assets, asset_tag_id TagID, real32 Value) {
	Assert(Assets->DEBUGAsset);
	asset_tag* Tag = Assets->Tags + Assets->DEBUGAsset->OnePassLastTagIndex++;
	Assets->DEBUGUsedTagCount++;

	Tag->ID = TagID;
	Tag->Value = Value;	
}

internal game_assets*
AllocateGameAssets(memory_arena* Arena, memory_index Size, transient_state* TranState) {
	game_assets* Assets = PushStruct(Arena, game_assets);

	SubArena(&Assets->Arena, Arena, Size);
	Assets->TranState = TranState;

	Assets->BitmapsCount = 255 * Asset_Count;
	Assets->BitmapInfos = PushArray(Arena, Assets->BitmapsCount, asset_bitmap_info);
	Assets->Bitmaps = PushArray(Arena, Assets->BitmapsCount, asset_slot);

	Assets->SoundCount = 255 * Asset_Count;
	Assets->SoundInfos = PushArray(Arena, Assets->SoundCount, asset_sound_info);
	Assets->Sounds = PushArray(Arena, Assets->SoundCount, asset_slot);


	Assets->TagCount = 1024 * Asset_Count;
	Assets->Tags = PushArray(Arena, Assets->TagCount, asset_tag);

	Assets->AssetCount = Assets->BitmapsCount + Assets->SoundCount;
	Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);

	Assets->DEBUGUsedBitmapCount = 1;
	Assets->DEBUGUsedSoundCount = 1;
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
	u32 MusicChunkSize = 2 * 48000;
	asset* LastMusic = 0;

	for (u32 FirstSampleIndex = 0; FirstSampleIndex < TotalSampleCount; FirstSampleIndex += MusicChunkSize) {
		u32 SampleCount = TotalSampleCount - FirstSampleIndex;
		if (SampleCount > MusicChunkSize) {
			SampleCount = MusicChunkSize;
		}
		asset *thisMusic = AddSoundAsset(Assets, "test3/music_test.wav", FirstSampleIndex, SampleCount);
		if (LastMusic) {
			Assets->SoundInfos[LastMusic->SlotID].NextIDToPlay.Value = thisMusic->SlotID;
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

	return(Assets);

}

internal uint32
GetFirstSlotFrom(game_assets *Assets, asset_type_id ID) {
	uint32 Result = 0;
	asset_type* Type = Assets->AssetTypes + ID;
	if (Type->FirstAssetIndex < Type->OnePassLastAssetIndex) {
		asset* Asset = Assets->Assets + Type->FirstAssetIndex;
		Result = Asset->SlotID;
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
		asset* Asset = Assets->Assets + Type->FirstAssetIndex + RandomChoice(Series, Count);
		Result = Asset->SlotID;
	}
	return(Result);
}

internal bitmap_id
GetRandomBitmapFrom(game_assets* Assets, asset_type_id ID, random_series* Series) {
	bitmap_id Result = {RandomAssetFrom(Assets, ID, Series)};
	return(Result);
}