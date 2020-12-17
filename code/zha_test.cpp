#include "zha_test.h"
#define STB_TRUETYPE_IMPLEMENTATION 1
#include "stb_truetype.h"

FILE* Out = 0;

struct loaded_sound {
    u32 ChannelCount;
    u32 SampleCount;
    s16* Samples[2];
    
    void* Free;
};
struct loaded_bitmap {
    s32 Width;
    s32 Height;
    s32 Pitch;
    void *Memory;
    void *Free;
};

struct loaded_font {
    u32 CodePointCount;
    r32 LineAdvance;
    bitmap_id* BitmapIDs;
    r32* HorizontalAdvance;
    
    stbtt_fontinfo Font;
    r32 Scale;
    b32 Loaded;
    void *Contents;
};

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

struct entire_file {
    u32 ContentsSize;
    void* Contents;
};

internal entire_file
ReadEntireFile(char* FileName) {
    FILE *File = fopen(FileName, "rb");
    entire_file Result = {};
    if (File) {
        fseek(File, 0, SEEK_END);
        Result.ContentsSize = ftell(File);
        fseek(File, 0, SEEK_SET);
        Result.Contents = malloc(Result.ContentsSize);
        fread(Result.Contents, Result.ContentsSize, 1, File);
        fclose(File);
    } else {
        printf("Cant open file at %s", FileName);
    }
    
    return (Result);
};

internal loaded_sound
LoadWAV(char* FileName, u32 FirstSampleIndex, u32 LoadSampleCount) {
	loaded_sound Result = {};
	entire_file ReadResult = ReadEntireFile(FileName);
	if (ReadResult.ContentsSize != 0) {
        
        Result.Free = ReadResult.Contents;
        
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
                    Assert(fmt->wBitsPerSample == 16);
                    
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
			Result.Samples[1] = SampleData + SampleCount;
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
LoadBMP(char* FileName, v2 AlignPercentage = v2{0.5f, 0.5f}) {
	loaded_bitmap Result = {};
	entire_file ReadResult = ReadEntireFile(FileName);
	if (ReadResult.ContentsSize != 0) {
        Result.Free = ReadResult.Contents;
		bitmap_header* Header = (bitmap_header*)ReadResult.Contents;
		uint32* Pixels = (uint32*)((uint8*)ReadResult.Contents + Header->BitmapOffset);
		Result.Height = Header->Height;
		Result.Width = Header->Width;
		Result.Memory = Pixels;
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

internal loaded_font*
InitFont(char* FileName, u32 CodePointCount) {
    loaded_font* Result = (loaded_font*)malloc(sizeof(loaded_font));
    entire_file TTFFile = ReadEntireFile(FileName);
    if (TTFFile.ContentsSize != 0) {
        Result->Contents = TTFFile.Contents;
        Result->Loaded = true;
        stbtt_InitFont(&Result->Font, (u8*)TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8*)TTFFile.Contents, 0));
        Result->Scale = stbtt_ScaleForPixelHeight(&Result->Font, 255.0f);
        
        int Ascent, Descent, Linegap;
        stbtt_GetFontVMetrics(&Result->Font, &Ascent, &Descent, &Linegap);
        
        Result->LineAdvance = Result->Scale * (Ascent - Descent + Linegap);
        Result->CodePointCount = CodePointCount;
        Result->BitmapIDs = (bitmap_id*)malloc(sizeof(bitmap_id) * CodePointCount);
        Result->HorizontalAdvance = (r32*)malloc(sizeof(r32) * CodePointCount * CodePointCount);
        
        //
        for (u32 CodePoint = 0; CodePoint < Result->CodePointCount; ++CodePoint) {
            for (u32 OtherCodePoint = 0; OtherCodePoint < Result->CodePointCount; ++OtherCodePoint) {
                Result->HorizontalAdvance[CodePoint * Result->CodePointCount + OtherCodePoint] = 10.0f;
            }
        }
    }
    return(Result);
}

internal void
FreeFont(loaded_font* Font) {
    free(Font->Contents);
}

internal loaded_bitmap
LoadGlyphBitmap(loaded_font* Font, u32 CodePoint, hha_asset* Asset) {
    
    loaded_bitmap Result = {};
    if (Font->Loaded) {
        int Width, Height, XOffset, YOffset, Ascent, Descent, Linegap;
        u8* MonoBitmap = stbtt_GetCodepointBitmap(&Font->Font, 0, Font->Scale,
                                                  CodePoint, &Width, &Height, &XOffset, &YOffset);
        stbtt_GetFontVMetrics(&Font->Font, &Ascent, &Descent, &Linegap);
        
        Result.Pitch = Width * BITMAP_BYTE_PER_PIXEL;
        Result.Width = Width;
        Result.Height = Height;
        Result.Memory = malloc(Result.Pitch * Height);
        Result.Free = Result.Memory;
        Asset->Bitmap.AlignPercentage[0] = 0.0f;
        Asset->Bitmap.AlignPercentage[1] = 1.0f - ((-YOffset - Font->Scale * Ascent) / Height);
        
        u8* Source = MonoBitmap;
        u8 *DestRow = (u8*)Result.Memory + (Height - 1) * Result.Pitch;
        for (int Y = 0; Y < Height; Y++) {
            u32 *Dest = (u32 *)DestRow;
            for (int X = 0; X < Width; X++) {
                //gamma correction
                r32 Gray = (r32)(*Source++ & 0xFF);
                
                v4 Texel = v4{255.0f, 255.0f, 255.0f, Gray};
                Texel = SRGBToLinear1(Texel);
                Texel.rgb *= Texel.a;
                
                Texel = Linear1ToSRGB(Texel);
                
				*Dest++ = (uint32)(Texel.a + 0.5f) << 24 |
					(uint32)(Texel.r + 0.5f) << 16 |
					(uint32)(Texel.g + 0.5f) << 8 |
					(uint32)(Texel.b + 0.5f) << 0;
            }
            DestRow -= Result.Pitch;
        }
        stbtt_FreeBitmap(MonoBitmap, 0);
        // todo
        //
    }
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

struct added_asset {
    u32 ID;
    hha_asset *HHA;
    asset_source* Source;
};

internal added_asset 
AddAsset(game_assets* Assets) {
    Assert(Assets->DEBUGAssetType);
    u32 Index = Assets->DEBUGAssetType->OnePassLastAssetIndex++;
	asset_source* Source = Assets->AssetSources + Index;
    hha_asset *Dest = Assets->Assets + Index;
    Dest->FirstTagIndex = Assets->TagCount;
	Dest->OnePassLastTagIndex = Dest->FirstTagIndex;
    Assets->AssetIndex = Index;
    added_asset Result = {};
    Result.ID = Index;
    Result.HHA = Dest;
    Result.Source = Source;
    return(Result);
}


internal bitmap_id
AddBitmapAsset(game_assets* Assets, char* FileName, r32 AlignmentPercentageX = 0.5f, r32 AlignmentPercentageY = 0.5f) {
    
    added_asset Asset = AddAsset(Assets);
    
    Asset.Source->Type = AssetType_Bitmap;
    Asset.Source->Bitmap.FileName = FileName;
    
	Asset.HHA->Bitmap.AlignPercentage[0] = AlignmentPercentageX;
	Asset.HHA->Bitmap.AlignPercentage[1] = AlignmentPercentageY;
	bitmap_id Result = {Asset.ID};
	return(Result);
}

internal font_id
AddFontAsset(game_assets* Assets, loaded_font *Font) {
    added_asset Asset = AddAsset(Assets);
    Asset.HHA->Font.LineAdvance = Font->LineAdvance;
    Asset.HHA->Font.CodePointCount = Font->CodePointCount;
    Asset.Source->Type = AssetType_Font;
    Asset.Source->Font.Font = Font;
    font_id Result = {Asset.ID};
	return(Result);
}


internal bitmap_id
AddCharacterAsset(game_assets* Assets, loaded_font* Font, u32 Codepoint) {
    
    added_asset Asset = AddAsset(Assets);
    Asset.Source->Type = AssetType_FontGlyph;
    Asset.Source->Glyph.Font = Font;
    Asset.Source->Glyph.Codepoint = Codepoint;
    // alignment get set when we load the font
    Asset.HHA->Bitmap.AlignPercentage[0] = 0.0f;
	Asset.HHA->Bitmap.AlignPercentage[1] = 0.0f;
	
    bitmap_id Result = { Asset.ID };
	return(Result);
}

internal sound_id
AddSoundAsset(game_assets* Assets, char* FileName, u32 FirstSampleIndex = 0, u32 LoadSampleCount = 0) {
	added_asset Asset = AddAsset(Assets);
    Asset.Source->Type = AssetType_Sound;
    Asset.Source->Sound.FileName = FileName;
    Asset.Source->Sound.FirstSampleIndex = FirstSampleIndex;
    
	Asset.HHA->FirstTagIndex = Assets->TagCount;
	Asset.HHA->OnePassLastTagIndex = Asset.HHA->FirstTagIndex;
	Asset.HHA->Sound.SampleCount = LoadSampleCount;
	Asset.HHA->Sound.Chain = HHASoundChain_none;
    
    sound_id Result = { Asset.ID };
	return(Result);
}


internal void
EndAssetType(game_assets* Assets) {
	Assert(Assets->DEBUGAssetType);
    Assets->AssetCount = Assets->DEBUGAssetType->OnePassLastAssetIndex;
    Assets->DEBUGAssetType = 0;
    Assets->AssetIndex = 0;
}

internal void
AddTag(game_assets* Assets, asset_tag_id TagID, real32 Value) {
	Assert(Assets->AssetIndex);
    
    hha_asset *HHA = Assets->Assets + Assets->AssetIndex;
    ++HHA->OnePassLastTagIndex;
    
	hha_tag* Tag = Assets->Tags + Assets->TagCount++;
	Tag->ID = TagID;
	Tag->Value = Value;
}

internal void
InitialAssets(game_assets *Assets) {
    Assets->AssetCount = 1;
	Assets->TagCount = 1;
    
	Assets->DEBUGAssetType = 0;
	Assets->AssetIndex = 0;
    
    Assets->AssetTypeCount = Asset_Count;
    memset(Assets->AssetTypes, 0, sizeof(Assets->AssetTypes)); 
    
}

internal void
PackHero() {
    game_assets Assets_;
	game_assets* Assets = &Assets_;
    InitialAssets(Assets);
    
	real32 AngleRight = 0.0f * Tau32;
	real32 AngleBack = 0.25f * Tau32;
	real32 AngleLeft = 0.5f * Tau32;
	real32 AngleFront = 0.75f * Tau32;
    
	BeginAssetType(Assets, Asset_Shadow);
	AddBitmapAsset(Assets, "test/test_hero_shadow.bmp", 0.5f, 0.156682029f);
	EndAssetType(Assets);
    
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
    
    
	Out = fopen("test1.hha", "wb");
	if (Out) {
        
		hha_header Header = {};
		Header.MagicValue = HHA_MAGIC_VALUE;
		Header.Version = HHA_VERSION;
        
		Header.AssetTypeCount = Asset_Count;
		Header.AssetCount = Assets->AssetCount;
        
		Header.TagCount = Assets->TagCount;
        
		u32 TagArraySize = Header.TagCount * sizeof(hha_tag);
		u32 AssetArraySize = Header.AssetCount * sizeof(hha_asset);
		u32 AssetTypeArraySize = Header.AssetTypeCount * sizeof(hha_asset_type);
        
		Header.Tags = sizeof(Header);
		Header.AssetTypes = Header.Tags + TagArraySize;
		Header.Assets = Header.AssetTypes + AssetTypeArraySize;
        
		
		fwrite(&Header, sizeof(Header), 1, Out);
		fwrite(Assets->Tags, TagArraySize, 1, Out);
		fwrite(Assets->AssetTypes, AssetTypeArraySize, 1, Out);
        fseek(Out, AssetArraySize, SEEK_CUR);
        for (u32 AssetIndex = 1; AssetIndex < Header.AssetCount; ++AssetIndex) {
            asset_source* Source = Assets->AssetSources + AssetIndex;
            hha_asset* Asset = Assets->Assets + AssetIndex;
            
            Asset->DataOffset = ftell(Out);
            
            if (Source->Type == AssetType_Sound) {
                loaded_sound Wave = LoadWAV(Source->Sound.FileName, Source->Sound.FirstSampleIndex, Asset->Sound.SampleCount);
                Asset->Sound.SampleCount = Wave.SampleCount;
                Asset->Sound.ChannelCount = Wave.ChannelCount;
                for (u32 ChannelIndex = 0; ChannelIndex < Asset->Sound.ChannelCount; ++ChannelIndex) {
                    fwrite(Wave.Samples[ChannelIndex], Wave.SampleCount*sizeof(s16), 1, Out);
                }
                free(Wave.Free);
            } else if (Source->Type == AssetType_Bitmap) {
                loaded_bitmap Bitmap = LoadBMP(Source->Bitmap.FileName);
                Asset->Bitmap.Dim[0] = Bitmap.Width;
                Asset->Bitmap.Dim[1] = Bitmap.Height;
                Assert((Bitmap.Width * 4) == Bitmap.Pitch);
                fwrite(Bitmap.Memory, Bitmap.Width * Bitmap.Height * 4, 1, Out);
                free(Bitmap.Free);
            }
        }
        fseek(Out, (u32)Header.Assets, SEEK_SET);
        fwrite(Assets->Assets, AssetArraySize, 1, Out);
		fclose(Out);
	}
	else {
		printf("Err: Could not open file\n");
	}
}

internal void
PackOtherAsset() {
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    InitialAssets(Assets);
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
    
    
    
    
    Out = fopen("test2.hha", "wb");
    if (Out) {
        
        hha_header Header = {};
        Header.MagicValue = HHA_MAGIC_VALUE;
        Header.Version = HHA_VERSION;
        
        Header.AssetTypeCount = Asset_Count;
        Header.AssetCount = Assets->AssetCount;
        
        Header.TagCount = Assets->TagCount;
        
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
        fseek(Out, AssetArraySize, SEEK_CUR);
        for (u32 AssetIndex = 1; AssetIndex < Header.AssetCount; ++AssetIndex) {
            asset_source* Source = Assets->AssetSources + AssetIndex;
            hha_asset* Asset = Assets->Assets + AssetIndex;
            
            Asset->DataOffset = ftell(Out);
            
            if (Source->Type == AssetType_Sound) {
                loaded_sound Wave = LoadWAV(Source->Sound.FileName, Source->Sound.FirstSampleIndex, Asset->Sound.SampleCount);
                Asset->Sound.SampleCount = Wave.SampleCount;
                Asset->Sound.ChannelCount = Wave.ChannelCount;
                for (u32 ChannelIndex = 0; ChannelIndex < Asset->Sound.ChannelCount; ++ChannelIndex) {
                    fwrite(Wave.Samples[ChannelIndex], Wave.SampleCount*sizeof(s16), 1, Out);
                }
                free(Wave.Free);
            } else if (Source->Type == AssetType_Font) {
                loaded_font *Font = Source->Font.Font;
                u32 CodePointSize = Font->CodePointCount * sizeof(bitmap_id);
                u32 HorizontalAdvanceSize = (u32)(Font->CodePointCount * Font->CodePointCount * sizeof(r32));
                fwrite(Font->BitmapIDs, CodePointSize, 1, Out);
                fwrite(Font->HorizontalAdvance, HorizontalAdvanceSize, 1, Out);
            } else {
                loaded_bitmap Bitmap = {};
                if (Source->Type == AssetType_FontGlyph) {
                    
                    Bitmap = LoadGlyphBitmap(Source->Glyph.Font, Source->Glyph.Codepoint, Asset);
                } else {
                    Assert(Source->Type == AssetType_Bitmap);
                    Bitmap = LoadBMP(Source->Bitmap.FileName);
                }
                Asset->Bitmap.Dim[0] = Bitmap.Width;
                Asset->Bitmap.Dim[1] = Bitmap.Height;
                
                
                Assert((Bitmap.Width * 4) == Bitmap.Pitch);
                fwrite(Bitmap.Memory, Bitmap.Width * Bitmap.Height * 4, 1, Out);
                free(Bitmap.Free);
            }
        }
        
        fseek(Out, (u32)Header.Assets, SEEK_SET);
        fwrite(Assets->Assets, AssetArraySize, 1, Out);
        fclose(Out);
    }
    else {
        printf("Err: Could not open file\n");
    }
}

internal void
PackFont() {
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    InitialAssets(Assets);
    
    loaded_font *DebugFont = InitFont("C:/Windows/Fonts/arial.ttf", '~' + 1);
    //AddCharacterAsset(Assets, "C:/Windows/Fonts/consola.ttf", Character);
    
    BeginAssetType(Assets, Asset_Font);
    AddFontAsset(Assets, DebugFont);
    EndAssetType(Assets);
    
    BeginAssetType(Assets, Asset_FontGlyph);
    
    for(u32 Character = '!'; Character <= '~'; ++Character) {
        DebugFont->BitmapIDs[Character] = AddCharacterAsset(Assets, DebugFont, Character);
    }
    
    EndAssetType(Assets);
    
    Out = fopen("font.hha", "wb");
    if (Out) {
        
        hha_header Header = {};
        Header.MagicValue = HHA_MAGIC_VALUE;
        Header.Version = HHA_VERSION;
        
        Header.AssetTypeCount = Asset_Count;
        Header.AssetCount = Assets->AssetCount;
        
        Header.TagCount = Assets->TagCount;
        
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
        fseek(Out, AssetArraySize, SEEK_CUR);
        for (u32 AssetIndex = 1; AssetIndex < Header.AssetCount; ++AssetIndex) {
            asset_source* Source = Assets->AssetSources + AssetIndex;
            hha_asset* Asset = Assets->Assets + AssetIndex;
            
            Asset->DataOffset = ftell(Out);
            
            if (Source->Type == AssetType_Sound) {
                loaded_sound Wave = LoadWAV(Source->Sound.FileName, Source->Sound.FirstSampleIndex, Asset->Sound.SampleCount);
                Asset->Sound.SampleCount = Wave.SampleCount;
                Asset->Sound.ChannelCount = Wave.ChannelCount;
                for (u32 ChannelIndex = 0; ChannelIndex < Asset->Sound.ChannelCount; ++ChannelIndex) {
                    fwrite(Wave.Samples[ChannelIndex], Wave.SampleCount*sizeof(s16), 1, Out);
                }
                free(Wave.Free);
            } else if (Source->Type == AssetType_Font) {
                loaded_font *Font = Source->Font.Font;
                u32 CodePointSize = Font->CodePointCount * sizeof(bitmap_id);
                u32 HorizontalAdvanceSize = (u32)(Font->CodePointCount * Font->CodePointCount * sizeof(r32));
                fwrite(Font->BitmapIDs, CodePointSize, 1, Out);
                fwrite(Font->HorizontalAdvance, HorizontalAdvanceSize, 1, Out);
            } else {
                loaded_bitmap Bitmap = {};
                if (Source->Type == AssetType_FontGlyph) {
                    
                    Bitmap = LoadGlyphBitmap(Source->Glyph.Font, Source->Glyph.Codepoint, Asset);
                } else {
                    Assert(Source->Type == AssetType_Bitmap);
                    Bitmap = LoadBMP(Source->Bitmap.FileName);
                }
                Asset->Bitmap.Dim[0] = Bitmap.Width;
                Asset->Bitmap.Dim[1] = Bitmap.Height;
                
                
                Assert((Bitmap.Width * 4) == Bitmap.Pitch);
                fwrite(Bitmap.Memory, Bitmap.Width * Bitmap.Height * 4, 1, Out);
                free(Bitmap.Free);
            }
        }
        fseek(Out, (u32)Header.Assets, SEEK_SET);
        fwrite(Assets->Assets, AssetArraySize, 1, Out);
        FreeFont(DebugFont);
        fclose(Out);
    }
    else {
        printf("Err: Could not open file\n");
    }
}



internal void
PackMusic() {
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    InitialAssets(Assets);
    
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
            Assets->Assets[LastMusic.Value].Sound.Chain = HHASoundChain_Advance;
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
    
    
    Out = fopen("test3.hha", "wb");
    if (Out) {
        
        hha_header Header = {};
        Header.MagicValue = HHA_MAGIC_VALUE;
        Header.Version = HHA_VERSION;
        
        Header.AssetTypeCount = Asset_Count;
        Header.AssetCount = Assets->AssetCount;
        
        Header.TagCount = Assets->TagCount;
        
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
        fseek(Out, AssetArraySize, SEEK_CUR);
        for (u32 AssetIndex = 1; AssetIndex < Header.AssetCount; ++AssetIndex) {
            asset_source* Source = Assets->AssetSources + AssetIndex;
            hha_asset* Asset = Assets->Assets + AssetIndex;
            
            Asset->DataOffset = ftell(Out);
            
            if (Source->Type == AssetType_Sound) {
                loaded_sound Wave = LoadWAV(Source->Sound.FileName, Source->Sound.FirstSampleIndex, Asset->Sound.SampleCount);
                Asset->Sound.SampleCount = Wave.SampleCount;
                Asset->Sound.ChannelCount = Wave.ChannelCount;
                for (u32 ChannelIndex = 0; ChannelIndex < Asset->Sound.ChannelCount; ++ChannelIndex) {
                    fwrite(Wave.Samples[ChannelIndex], Wave.SampleCount*sizeof(s16), 1, Out);
                }
                free(Wave.Free);
            } else if (Source->Type == AssetType_Bitmap) {
                loaded_bitmap Bitmap = LoadBMP(Source->Bitmap.FileName);
                Asset->Bitmap.Dim[0] = Bitmap.Width;
                Asset->Bitmap.Dim[1] = Bitmap.Height;
                Assert((Bitmap.Width * 4) == Bitmap.Pitch);
                fwrite(Bitmap.Memory, Bitmap.Width * Bitmap.Height * 4, 1, Out);
                free(Bitmap.Free);
            }
        }
        fseek(Out, (u32)Header.Assets, SEEK_SET);
        fwrite(Assets->Assets, AssetArraySize, 1, Out);
        fclose(Out);
    }
    else {
        printf("Err: Could not open file\n");
    }
}


int main(int ArgCount, char **Args) {
	PackMusic();
    PackOtherAsset();
    PackHero();
	printf("hello world");
    PackFont();
	return(0);
} 