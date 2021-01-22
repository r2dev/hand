#pragma once

#define ONE_PAST_MAX_FONT_CODEPOINT (0x10FFFF + 1)
enum asset_type_id {
	Asset_None,
    
	Asset_Shadow,
	Asset_Tree,
	Asset_Sword,
    
	Asset_Grass,
	Asset_Ground,
	Asset_Tuft,
    
	Asset_Head,
	Asset_Cape,
	Asset_Torso,
    
    Asset_Font,
    Asset_FontGlyph,
    
	Asset_Music,
    //Asset_Forget,
	Asset_Bloop,
    
    Asset_1,
    Asset_2,
    Asset_3,
    Asset_4,
    Asset_5,
    
    Asset_OpeningCutScene,
    
    Asset_Count,
};

enum asset_font_type {
    FontType_Default = 0,
    FontType_Debug = 10,
};

enum asset_tag_id {
	Tag_Smoothness,
	Tag_Flagness,
	Tag_FaceDirection,
    
    Tag_UnicodeCodepoint,
    Tag_FontType,
    
    Tag_ShotIndex,
    Tag_LayerIndex,
    
    
	Tag_Count
};


#pragma pack(push, 1)

enum {
    HHASoundChain_none,
    HHASoundChain_loop,
    HHASoundChain_Advance
};
#define RIFF_CODE(a, b, c, d) (((u32)(a) << 0) | ((u32)(b) << 8) | ((u32)(c) << 16) | ((u32)(d) << 24))

struct hha_header {
#define HHA_MAGIC_VALUE RIFF_CODE('h', 'h', 'a', 'f')
	u32 MagicValue;
#define HHA_VERSION 0
	u32 Version;
	
	u32 AssetTypeCount;
	u32 AssetCount;
	u32 TagCount;
    
	u64 AssetTypes;
	u64 Assets;
	u64 Tags;
};

struct hha_bitmap {
	u32 Dim[2];
	r32 AlignPercentage[2];
    
    // u32 Pixel[Dim[1]][Dim[0]]
};

struct hha_sound {
    u32 ChannelCount;
	u32 SampleCount;
    u32 Chain;
    
    // s16 Channels[ChannelCount][SampleCount]
};

struct hha_font_glyph {
    u32 UnicodeCodePoint;
    bitmap_id BitmapID;
};

struct hha_font {
    u32 GlyphCount;
    r32 Ascent;
    r32 Descent;
    r32 ExternalLeading;
    u32 OnePastHighestCodePoint;
    
    //hha_font_glyph FontGlyphs[GlyphCount];
    //r32 HorizontalAdvance[GlyphCount][GlyphCount];
    
};

struct hha_asset_type {
	u32 TypeID;
	u32 FirstAssetIndex;
	u32 OnePassLastAssetIndex;
};

struct hha_asset {
	u64 DataOffset;
	u32 FirstTagIndex;
	u32 OnePassLastTagIndex;
	union {
		hha_bitmap Bitmap;
		hha_sound Sound;
        hha_font Font;
	};
};


struct hha_tag {
	u32 ID;
	r32 Value;
};

#pragma pack(pop)