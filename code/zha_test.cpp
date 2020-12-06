#include "zha_test.h"

FILE* Out = 0;


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
        
		for (riff_iterator Iter = ParseChunkAt(Header + 1, (uint8*)(Header + 1) + Header->Size - 4);
             IsValid(Iter);
             Iter = NextChunk(Iter)
             ) {
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






internal void
BeginAssetType(game_assets* Assets, asset_type_id ID) {
	Assert(Assets->DEBUGAssetType == 0);
    
	Assets->DEBUGAssetType = Assets->AssetTypes + ID;
	Assets->DEBUGAssetType->TypeID = ID;
	Assets->DEBUGAssetType->FirstAssetIndex = Assets->AssetCount++;
	Assets->DEBUGAssetType->OnePassLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

internal bitmap_id
AddBitmapAsset(game_assets* Assets, char* FileName, r32 AlignmentPercentageX = 0.5f, r32 AlignmentPercentageY = 0.5f) {
	bitmap_id Result = { Assets->DEBUGAssetType->OnePassLastAssetIndex++ };
	Assert(Assets->DEBUGAssetType);
	asset* Asset = Assets->Assets + Result.Value;
	Asset->FirstTagIndex = Assets->TagCount++;
	Asset->OnePassLastTagIndex = Asset->FirstTagIndex;
	asset_bitmap_info* Info = &Assets->Assets[Result.Value].Bitmap;
	Info->AlignPercentageX = AlignmentPercentageX;
	Info->AlignPercentageY = AlignmentPercentageY;
	Info->FileName = FileName;
	Assets->DEBUGAsset = Asset;
	return(Result);
}

internal sound_id
AddSoundAsset(game_assets* Assets, char* FileName, u32 FirstSampleIndex = 0, u32 LoadSampleCount = 0) {
	sound_id Result = { Assets->DEBUGAssetType->OnePassLastAssetIndex++ };
	Assert(Assets->DEBUGAssetType);
	asset* Asset = Assets->Assets + Result.Value;
	Asset->FirstTagIndex = Assets->TagCount++;
	Asset->OnePassLastTagIndex = Asset->FirstTagIndex;
	asset_sound_info* Info = &Assets->Assets[Result.Value].Sound;
	Info->FileName = FileName;
	Info->FirstSampleIndex = FirstSampleIndex;
	Info->SampleCount = LoadSampleCount;
	Info->NextIDToPlay.Value = 0;
	Assets->DEBUGAsset = Asset;
	return(Result);
}


internal void
EndAssetType(game_assets* Assets) {
	Assert(Assets->DEBUGAssetType);
	Assets->DEBUGAssetType = 0;
}

internal void
AddTag(game_assets* Assets, asset_tag_id TagID, real32 Value) {
	Assert(Assets->DEBUGAsset);
	hha_tag* Tag = Assets->Tags + Assets->TagCount++;
    
	Tag->ID = TagID;
	Tag->Value = Value;
}


int main(int ArgCount, char **Args) {
	game_assets Assets_;
	game_assets* Assets = &Assets_;
    
	Assets->AssetCount = 1;
	Assets->TagCount = 1;
	
	Assets->DEBUGAssetType = 0;
	Assets->DEBUGAsset = 0;
	BeginAssetType(Assets, Asset_Shadow);
	AddBitmapAsset(Assets, "test/test_hero_shadow.bmp", 0.5f, 0.156682029f);
	EndAssetType(Assets);
    
	BeginAssetType(Assets, Asset_Tree);
	AddBitmapAsset(Assets, "test2/tree00.bmp", 0.493827164f, 0.295652181f);
	EndAssetType(Assets);
    
	BeginAssetType(Assets, Asset_Sword);
	AddBitmapAsset(Assets, "test2/rock02.bmp", 0.5f, 0.65625f);
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
    
    
    
	BeginAssetType(Assets, Asset_Head);
	AddBitmapAsset(Assets, "test/test_hero_right_head.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleRight);
	AddBitmapAsset(Assets, "test/test_hero_back_head.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleBack);
	AddBitmapAsset(Assets, "test/test_hero_left_head.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleLeft);
	AddBitmapAsset(Assets, "test/test_hero_front_head.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleFront);
	EndAssetType(Assets);
    
	BeginAssetType(Assets, Asset_Cape);
	AddBitmapAsset(Assets, "test/test_hero_right_cape.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleRight);
	AddBitmapAsset(Assets, "test/test_hero_back_cape.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleBack);
	AddBitmapAsset(Assets, "test/test_hero_left_cape.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleLeft);
	AddBitmapAsset(Assets, "test/test_hero_front_cape.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleFront);
	EndAssetType(Assets);
    
	BeginAssetType(Assets, Asset_Torso);
	AddBitmapAsset(Assets, "test/test_hero_right_torso.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleRight);
	AddBitmapAsset(Assets, "test/test_hero_back_torso.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleBack);
	AddBitmapAsset(Assets, "test/test_hero_left_torso.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleLeft);
	AddBitmapAsset(Assets, "test/test_hero_front_torso.bmp", 0.5f, 0.156682029f);
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
		if (LastMusic.Value) {
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
    
    
    
	Out = fopen("test.hha", "wb");
	if (Out) {
        
		hha_header Header = {};
		Header.MagicValue = HHA_MAGIC_VALUE;
		Header.Version = HHA_VERSION;
        
		Header.AssetTypeCount = Asset_Count;
		Header.AssetCount = 1;
        
		Header.TagCount = 0;
        
		u32 TagArraySize = Header.TagCount * sizeof(hha_tag);
		u32 AssetArraySize = Header.AssetCount * sizeof(hha_asset);
		u32 AssetTypeArraySize = Header.AssetTypeCount * sizeof(hha_asset_type);
        
		Header.Tags = sizeof(Header);
		Header.AssetTypes = Header.Tags + TagArraySize;
		Header.Assets = Header.AssetTypes + AssetTypeArraySize;
        
		
		fwrite(&Header, sizeof(Header), 1, Out);
		fwrite(Assets->Tags, TagArraySize, 1, Out);
		fwrite(Assets->AssetTypes, AssetTypeArraySize, 1, Out);
		//fwrite(Header.Assets, AssetArraySize, 1, Out);
        
        
		fclose(Out);
	}
	else {
		printf("Err: Could not open file\n");
	}
	printf("hello world");
    
    
    
	
	return(0);
} 