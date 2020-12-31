#pragma once


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
	real32 Scale;
};

struct render_entity_basis {
	render_basis* Basis;
	v3 Offset;
};

struct render_transform {
	real32 DistanceAboveTarget;
	real32 FocalLength;
    
	real32 Scale;
	v3 OffsetP;
	v2 ScreenCenter;
	
	real32 MetersToPixels;
    
	bool32 Orthographic;
};


struct render_group {
	uint32 MaxPushBufferSize;
	
	uint32 PushBufferSize;
	uint8* PushBufferBase;
	
	real32 GlobalAlpha;
	v2 MonitorHalfDimInMeters;
	
	render_transform Transform;
	struct game_assets* Assets;
    
	uint32 MissingResourceCount;
    u32 GenerationID;
    b32 RenderInBackground;
    
    b32 InsideRender;
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
	real32 Scale;
	bool32 Valid;
};

struct used_bitmap_dim {
    v2 Size;
	v2 Align;
	v3 P;
	entity_basis_p_result Basis;
};

void DrawRectangle2(loaded_bitmap* Buffer, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture, real32 PixelsToMeters, rectangle2i ClipRect, bool32 Even);