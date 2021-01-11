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
    u32 MaxGlyphCount;
    u32 GlyphCount;
    
    u32 MinCodePoint;
    u32 MaxCodePoint;
    
    r32 Ascent;
    r32 Descent;
    r32 ExternalLeading;
    
    u32 OnePastHighestCodePoint;
    
    // bitmap_id* BitmapIDs;
    hha_font_glyph* Glyphs;
    r32* HorizontalAdvance;
    u32 *GlyphIndexFromCodePoint;
    
    stbtt_fontinfo Font;
    r32 Scale;
    b32 Loaded;
    void *Contents;
};

#pragma pack(push, 1)
struct bitmap_header {
	s16 FileType;
	u32 FileSize;
	s16 Reserved1;
	s16 Reserved2;
	u32 BitmapOffset;
	u32 Size;
	s32 Width;
	s32 Height;
	s16 Planes;
	s16 BitsPerPixel;
	u32 Compression;
	u32 SizeOfBitmap;
	s32 HorzResolution;
	s32 VertResolution;
	u32 ColorsUsed;
	u32 ColorsImportant;
	u32 RedMask;
	u32 GreenMask;
	u32 BlueMask;
};


struct WAVE_header {
	u32 RIFFID;
	u32 Size;
	u32 WaveID;
};

#define RIFF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))
enum {
	WAVE_ChunkID_fmt = RIFF_CODE('f', 'm', 't', ' '),
	WAVE_ChunkID_data = RIFF_CODE('d', 'a', 't', 'a'),
	WAVE_ChunkID_RIFF = RIFF_CODE('R', 'I', 'F', 'F'),
	WAVE_ChunkID_WAVE = RIFF_CODE('W', 'A', 'V', 'E'),
};	

struct WAVE_chunk {
	u32 ID;
	u32 Size;
};

struct WAVE_fmt {
	s16 wFormatTag;
	s16 nChannels;
	u32 nSamplesPerSec;
	u32 nAvgBytesPerSec;
	s16 nBlockAlign;
	s16 wBitsPerSample;
	s16 cbSize;
	s16 wValidBitsPerSample;
	u32 dwChannelMask;
	u8 SubFormat[16];
    
};

#pragma pack(pop)

struct riff_iterator {
	u8* At;
	u8* Stop;
};

inline riff_iterator
ParseChunkAt(void* At, void* Stop) {
	riff_iterator Iter;
	Iter.At = (u8*)At;
	Iter.Stop = (u8*)Stop;
	return(Iter);
    
}

inline b32
IsValid(riff_iterator Iter) {
	b32 Result = (Iter.At < Iter.Stop);
	return(Result);
}

inline riff_iterator
NextChunk(riff_iterator Iter) {
	WAVE_chunk* Chunk = (WAVE_chunk*)Iter.At;
	// padding
	u32 Size = (Chunk->Size + 1) & ~1;
	Iter.At += sizeof(WAVE_chunk) + Size;
	return(Iter);
}

inline u32
GetType(riff_iterator Iter) {
	WAVE_chunk* Chunk = (WAVE_chunk*)Iter.At;
	u32 Result = Chunk->ID;
	return(Result);
}



inline void*
GetChunkData(riff_iterator Iter) {
	void* Result = (Iter.At + sizeof(WAVE_chunk));
	return(Result);
}

inline u32
GetChunkDataSize(riff_iterator Iter) {
	WAVE_chunk* Chunk = (WAVE_chunk*)Iter.At;
	u32 Result = Chunk->Size;
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
		u32 ChannelCount = 0;
		u32 SampleDataSize = 0;
		s16* SampleData = 0;
        
		for (riff_iterator Iter = ParseChunkAt(Header + 1, (u8*)(Header + 1) + Header->Size - 4);
             IsValid(Iter);
             Iter = NextChunk(Iter)
             ) {
			switch (GetType(Iter)) {
                
                case WAVE_ChunkID_data: {
                    SampleData = (s16*)GetChunkData(Iter);
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
		u32 SampleCount = SampleDataSize / (ChannelCount * sizeof(s16));
        
		if (ChannelCount == 1) {
			Result.Samples[0] = SampleData;
			Result.Samples[1] = 0;
		}
		else if (ChannelCount == 2) {
			Result.Samples[0] = SampleData;
			Result.Samples[1] = SampleData + SampleCount;
			for (u32 SampleIndex = 0; SampleIndex < SampleCount; ++SampleIndex) {
				s16 Source = SampleData[2 * SampleIndex];
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
		u32* Pixels = (u32*)((u8*)ReadResult.Contents + Header->BitmapOffset);
		Result.Height = Header->Height;
		Result.Width = Header->Width;
		Result.Memory = Pixels;
		u32 RedMask = Header->RedMask;
		u32 GreenMask = Header->GreenMask;
		u32 BlueMask = Header->BlueMask;
		u32 AlphaMask = ~(RedMask | GreenMask | BlueMask);
        
		bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
		bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
		bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
		bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);
		Assert(RedScan.Found);
		Assert(GreenScan.Found);
		Assert(BlueScan.Found);
		Assert(AlphaScan.Found);
        
		s32 RedShiftDown = (s32)RedScan.Index;
		s32 GreenShiftDown = (s32)GreenScan.Index;
		s32 BlueShiftDown = (s32)BlueScan.Index;
		s32 AlphaShiftDown = (s32)AlphaScan.Index;
        
		u32* SourceDest = Pixels;
		for (s32 Y = 0;
             Y < Header->Height;
             ++Y)
		{
			for (s32 X = 0;
                 X < Header->Width;
                 ++X)
			{
				u32 C = *SourceDest;
				v4 Texel = {
					(r32)((C & RedMask) >> RedShiftDown),
					(r32)((C & GreenMask) >> GreenShiftDown),
					(r32)((C & BlueMask) >> BlueShiftDown),
					(r32)((C & AlphaMask) >> AlphaShiftDown)
				};
				Texel = SRGBToLinear1(Texel);
				Texel.rgb *= Texel.a;
				Texel = Linear1ToSRGB(Texel);
                
				*SourceDest++ = ((u32)(Texel.a + 0.5f) << 24) |
					((u32)(Texel.r + 0.5f) << 16) |
					((u32)(Texel.g + 0.5f) << 8) |
					((u32)(Texel.b + 0.5f) << 0);
			}
		}
	}
	Result.Pitch = Result.Width * BITMAP_BYTE_PER_PIXEL;
#if 0
	Result.Pitch = -Result.Width * BITMAP_BYTE_PER_PIXEL;
	Result.Memory = (u8*)Result.Memory - Result.Pitch * (Result.Height - 1);
#endif
    
	return(Result);
}

internal loaded_font*
InitFont(char* FileName, r32 PixelHeight) {
    loaded_font* Font = (loaded_font*)malloc(sizeof(loaded_font));
    entire_file TTFFile = ReadEntireFile(FileName);
    if (TTFFile.ContentsSize != 0) {
        Font->Contents = TTFFile.Contents;
        Font->Loaded = true;
        stbtt_InitFont(&Font->Font, (u8*)TTFFile.Contents, stbtt_GetFontOffsetForIndex((u8*)TTFFile.Contents, 0));
        Font->Scale = stbtt_ScaleForPixelHeight(&Font->Font, PixelHeight);
        
        int Ascent, Descent, Linegap;
        stbtt_GetFontVMetrics(&Font->Font, &Ascent, &Descent, &Linegap);
        
        Font->Ascent = Font->Scale * Ascent;
        // (ren) store as positive value for now, change if needed in the future
        Font->Descent = -Font->Scale * Descent;
        Font->ExternalLeading = Font->Scale * Linegap;
        
        Font->GlyphCount = 0;
        Font->MaxGlyphCount = 5000;
        Font->MaxCodePoint = INT_MAX;
        Font->MinCodePoint = 0;
        Font->OnePastHighestCodePoint = 0;
        
        Font->Glyphs = (hha_font_glyph*)malloc(sizeof(hha_font_glyph) * Font->MaxGlyphCount);
        // (ren) first glyph should be empty
        Font->Glyphs[0].UnicodeCodePoint = 0;
        Font->Glyphs[0].BitmapID = {};
        
        u32 HorizontalAdvanceSize = sizeof(r32) * Font->MaxGlyphCount * Font->MaxGlyphCount;
        Font->HorizontalAdvance = (r32*)malloc(HorizontalAdvanceSize);
        memset(Font->HorizontalAdvance, 0, HorizontalAdvanceSize);
        
        u32 GlyphIndexFromCodePointSize = ONE_PAST_MAX_FONT_CODEPOINT * sizeof(u32);
        Font->GlyphIndexFromCodePoint = (u32*)malloc(GlyphIndexFromCodePointSize);
        memset(Font->GlyphIndexFromCodePoint, 0, GlyphIndexFromCodePointSize);
        
    }
    return(Font);
}

internal void
ApplyKerningValue(loaded_font* Font) {
    int KerningTableLength = stbtt_GetKerningTableLength(&Font->Font);
    stbtt_kerningentry *Entries = (stbtt_kerningentry *)malloc(sizeof(stbtt_kerningentry) * KerningTableLength);
    stbtt_GetKerningTable(&Font->Font, Entries, KerningTableLength);
    u32 CodePointOffset = '!' - stbtt_FindGlyphIndex(&Font->Font, '!');
    for (int KerningTableIndex = 0; KerningTableIndex < KerningTableLength; ++KerningTableIndex) {
        stbtt_kerningentry *Entry = Entries + KerningTableIndex;
        u32 CodePoint1 = Entry->glyph1 + CodePointOffset;
        u32 CodePoint2 = Entry->glyph2 + CodePointOffset;
        int glyph1 = Font->GlyphIndexFromCodePoint[CodePoint1];
        int glyph2 = Font->GlyphIndexFromCodePoint[CodePoint2];
        if (glyph1 > 0 && glyph2 > 0 && (u32)glyph1 < Font->MaxGlyphCount && (u32)glyph2 < Font->MaxGlyphCount) {
            Font->HorizontalAdvance[glyph1 * Font->MaxGlyphCount + glyph2] += Font->Scale * (r32)Entry->advance;
        }
    }
    free(Entries);
}

internal void
FreeFont(loaded_font* Font) {
    if (Font) {
        free(Font->Contents);
        free(Font->HorizontalAdvance);
        free(Font->GlyphIndexFromCodePoint);
        free(Font);
    }
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
        Asset->Bitmap.AlignPercentage[0] = 0;
        Asset->Bitmap.AlignPercentage[1] = 1.0f - ((-YOffset) / (r32)Result.Height);
        
        u8* Source = MonoBitmap;
        u8 *DestRow = (u8*)Result.Memory + (Height - 1) * Result.Pitch;
        for (int Y = 0; Y < Height; Y++) {
            u32 *Dest = (u32 *)DestRow;
            for (int X = 0; X < Width; X++) {
                r32 Gray = (r32)(*Source++ & 0xFF);
                v4 Texel = v4{255.0f, 255.0f, 255.0f, Gray};
                Texel = SRGBToLinear1(Texel);
                Texel.rgb *= Texel.a;
                Texel = Linear1ToSRGB(Texel);
                *Dest++ = (u32)(Texel.a + 0.5f) << 24 |
                    (u32)(Texel.r + 0.5f) << 16 |
                    (u32)(Texel.g + 0.5f) << 8 |
                    (u32)(Texel.b + 0.5f) << 0;
            }
            DestRow -= Result.Pitch;
        }
        stbtt_FreeBitmap(MonoBitmap, 0);
        
        int AdvanceWidth, LeftSideBearing;
        stbtt_GetCodepointHMetrics(&Font->Font, CodePoint, &AdvanceWidth, &LeftSideBearing);
        u32 GlyphIndex = Font->GlyphIndexFromCodePoint[CodePoint];
        
        for (u32 OtherGlyphIndex = 0; OtherGlyphIndex < Font->MaxGlyphCount; ++OtherGlyphIndex) {
            // todo
            Font->HorizontalAdvance[GlyphIndex * Font->MaxGlyphCount + OtherGlyphIndex] += (r32)(Font->Scale * (AdvanceWidth - LeftSideBearing));
            if (OtherGlyphIndex != 0) {
                Font->HorizontalAdvance[OtherGlyphIndex * Font->MaxGlyphCount + GlyphIndex] += (r32)(Font->Scale * LeftSideBearing);
            }
        }
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
    Asset.HHA->Font.Ascent = Font->Ascent;
    Asset.HHA->Font.Descent = Font->Descent;
    Asset.HHA->Font.ExternalLeading = Font->ExternalLeading;
    Asset.HHA->Font.GlyphCount = Font->GlyphCount;
    Asset.HHA->Font.OnePastHighestCodePoint = Font->OnePastHighestCodePoint;
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
    
    bitmap_id Result = {Asset.ID};
    
    Assert(Font->GlyphCount < Font->MaxGlyphCount);
    u32 GlyphIndex = Font->GlyphCount++;
    hha_font_glyph *Glyph = Font->Glyphs + GlyphIndex;
    Glyph->UnicodeCodePoint = Codepoint;
    Glyph->BitmapID = Result;
    Font->GlyphIndexFromCodePoint[Codepoint] = GlyphIndex;
    
    if (Codepoint >= Font->OnePastHighestCodePoint) {
        Font->OnePastHighestCodePoint = Codepoint + 1;
    }
    
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
AddTag(game_assets* Assets, asset_tag_id TagID, r32 Value) {
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
WriteHHA(game_assets* Assets, char* FileName) {
    Out = fopen(FileName, "wb");
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
                u32 GlyphSize = Font->GlyphCount * sizeof(hha_font_glyph);
                fwrite(Font->Glyphs, GlyphSize, 1, Out);
                ApplyKerningValue(Font);
                
                u8* HorizontalAdvance = (u8*)Font->HorizontalAdvance;
                for (u32 GlyphIndex = 0; GlyphIndex < Font->GlyphCount; ++GlyphIndex) {
                    u32 HorizontalAdvanceSliceSize = Font->GlyphCount * sizeof(r32);
                    fwrite(HorizontalAdvance, HorizontalAdvanceSliceSize, 1, Out);
                    HorizontalAdvance += sizeof(r32) * Font->MaxGlyphCount;
                }
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
        //FreeFont(DebugFont);
        fclose(Out);
    }
    else {
        printf("Err: Could not open file\n");
    }
}

internal void
PackHero() {
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    InitialAssets(Assets);
    
    r32 AngleRight = 0.0f * Tau32;
    r32 AngleBack = 0.25f * Tau32;
    r32 AngleLeft = 0.5f * Tau32;
    r32 AngleFront = 0.75f * Tau32;
    
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
    
    
    WriteHHA(Assets, "test1.hha");
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
    
    WriteHHA(Assets, "test2.hha");
}

internal void
PackFont()
{
    game_assets Assets_;
    game_assets* Assets = &Assets_;
    InitialAssets(Assets);
    loaded_font* Fonts[] = {
        InitFont("C:/Windows/Fonts/arial.ttf", 128),
        InitFont("C:/Windows/Fonts/consola.ttf", 24),
        //InitFont("C:/Windows/Fonts/Roboto-Regular.ttf", 24),
    };
    
    BeginAssetType(Assets, Asset_FontGlyph);
    for (u32 FontIndex = 0; FontIndex < ArrayCount(Fonts); ++FontIndex) {
        loaded_font *Font = Fonts[FontIndex];
        AddCharacterAsset(Assets, Font, 0x5c0f);
        AddCharacterAsset(Assets, Font, ' ');
        
        for(u32 Character = '!'; Character <= '~'; ++Character) {
            AddCharacterAsset(Assets, Font, Character);
        }
        AddCharacterAsset(Assets, Font, 0x8033);
        AddCharacterAsset(Assets, Font, 0x6728);
        AddCharacterAsset(Assets, Font, 0x514e);
    }
    EndAssetType(Assets);
    BeginAssetType(Assets, Asset_Font);
    
    AddFontAsset(Assets, Fonts[0]);
    AddTag(Assets, Tag_FontType, FontType_Default);
    AddFontAsset(Assets, Fonts[1]);
    AddTag(Assets, Tag_FontType, FontType_Debug);
    EndAssetType(Assets);
    WriteHHA(Assets,"font.hha");
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
    
    
    WriteHHA(Assets, "test3.hha");
    
}


int main(int ArgCount, char **Args) {
    PackMusic();
    PackOtherAsset();
    PackHero();
    PackFont();
    return(0);
}