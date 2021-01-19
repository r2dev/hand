#pragma once

struct loaded_bitmap {
    void* Memory;
	v2 AlignPercentage;
	r32 WidthOverHeight;
    
	s32 Width;
	s32 Height;
	s32 Pitch;
    
    // opengl handle texture
    u32 Handle;
};

struct environment_map {
	loaded_bitmap LOD[4];
	r32 Pz;
};

enum render_group_entry_type {
	RenderGroupEntryType_render_entry_clear,
	RenderGroupEntryType_render_entry_rectangle,
	RenderGroupEntryType_render_entry_bitmap,
	RenderGroupEntryType_render_entry_coordinate_system
};

struct render_group_entry_header {
	render_group_entry_type Type;
};

struct render_basis {
	v3 P;
	r32 Scale;
};

struct render_entity_basis {
	render_basis* Basis;
	v3 Offset;
};

struct render_transform {
	r32 DistanceAboveTarget;
	r32 FocalLength;
    
	r32 Scale;
	v3 OffsetP;
	v2 ScreenCenter;
	
	r32 MetersToPixels;
    
	b32 Orthographic;
};


struct render_group {
    b32 IsHardware;
	
	r32 GlobalAlpha;
	v2 MonitorHalfDimInMeters;
	
	render_transform Transform;
	struct game_assets* Assets;
    
	u32 MissingResourceCount;
    u32 GenerationID;
    b32 RenderInBackground;
    
    game_render_commands *Commands;
};

struct render_entry_clear {
	v4 Color;
};

struct render_entry_rectangle {
	v2 P;
	v4 Color;
	v2 Dim;
    
};

struct render_entry_bitmap {
	v2 P;
	loaded_bitmap* Bitmap;
	v4 Color;
	v2 Size;
};

struct render_entry_coordinate_system {
	v2 Origin;
	v2 AxisX;
	v2 AxisY;
	v4 Color;
    
	loaded_bitmap* Texture;
	loaded_bitmap* NormalMap;
    
	environment_map* Top;
	environment_map* Middle;
	environment_map* Bottom;
};

struct entity_basis_p_result {
	v2 P;
	r32 Scale;
	b32 Valid;
};

struct used_bitmap_dim {
    v2 Size;
	v2 Align;
	v3 P;
	entity_basis_p_result Basis;
};

void DrawRectangle2(loaded_bitmap* Buffer, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture, r32 PixelsToMeters, rectangle2i ClipRect, b32 Even);