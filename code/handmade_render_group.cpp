#define IGNORED_TIMED_FUNCTION() TIMED_FUNCTION()

inline entity_basis_p_result
GetRenderEntityBasisP(render_transform* Transform, v3 OriginP) {
	entity_basis_p_result Result = {};
    
	v3 P = V3(OriginP.xy, 0.0f) + Transform->OffsetP;
    
	if (Transform->Orthographic == false) {
        r32 DistanceAboveTarget = Transform->DistanceAboveTarget;
        
        DEBUG_IF(Renderer_UseDebugCamera)
        {
            DEBUG_VARIABLE(r32, Renderer_Camera, DebugCameraDistance);
            DistanceAboveTarget += DebugCameraDistance;
        }
        
		r32 DistancePz = DistanceAboveTarget - P.z;
		v3 RawXY = V3(P.xy, 1.0f);
		r32 NearClipPlane = 0.2f;
        
		if (DistancePz > NearClipPlane) {
			v3 ProjectedXY = 1.0f / DistancePz * Transform->FocalLength * RawXY;
			Result.Scale = Transform->MetersToPixels * ProjectedXY.z;
			v2 EntityGround = Transform->ScreenCenter + Transform->MetersToPixels * ProjectedXY.xy;
			Result.P = EntityGround;
			Result.Valid = true;
		}
	}
	else {
		Result.Scale = Transform->MetersToPixels;
		Result.Valid = true;
		Result.P = Transform->ScreenCenter + Transform->MetersToPixels * P.xy;
	}
    
	return(Result);
}

#define PushRenderElement(Group, type)(type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type)


inline void*
PushRenderElement_(render_group* Group, u32 Size, render_group_entry_type Type) {
    game_render_commands *Commands = Group->Commands;
    
	void* Result = 0;
	Size += sizeof(render_group_entry_header);
	if (Commands->PushBufferSize + Size < Commands->MaxPushBufferSize) {
		render_group_entry_header* Header = (render_group_entry_header*)(Commands->PushBufferBase + Commands->PushBufferSize);
		Header->Type = Type;
		Result = Header + 1;
		Commands->PushBufferSize += Size;
	}
	else {
		InvalidCodePath;
	}
	return(Result);
}

inline used_bitmap_dim
GetBitmapDim(render_group* Group, loaded_bitmap* Bitmap, r32 Height, v3 Offset, r32 CAlign) {
    used_bitmap_dim Result;
    Result.Size = v2{ Height * Bitmap->WidthOverHeight, Height };
	Result.Align = CAlign * Hadamard(Result.Size, Bitmap->AlignPercentage);
	Result.P = Offset - V3(Result.Align, 0);
	Result.Basis = GetRenderEntityBasisP(&Group->Transform, Result.P);
    
    return(Result);
}

inline void
PushBitmap(render_group* Group, loaded_bitmap* Bitmap, r32 Height, v3 Offset, v4 Color = v4{ 1.0f, 1.0, 1.0f, 1.0f }, r32 CAlign = 1.0f) {
	used_bitmap_dim BitmapDim = GetBitmapDim(Group, Bitmap, Height, Offset, CAlign);
	if (BitmapDim.Basis.Valid) {
		render_entry_bitmap* Entry = PushRenderElement(Group, render_entry_bitmap);
		if (Entry) {
			Entry->P = BitmapDim.Basis.P;
			Entry->Bitmap = Bitmap;
			Entry->Color = Group->GlobalAlpha * Color;
			Entry->Size = BitmapDim.Basis.Scale * BitmapDim.Size;
		}
	}
}


inline void
PushBitmap(render_group* Group, bitmap_id ID, r32 Height, v3 Offset, v4 Color = v4{ 1.0f, 1.0, 1.0f, 1.0f }, r32 CAlign = 1.0f) {
	loaded_bitmap* Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
    
    if (Group->RenderInBackground && !Bitmap) {
        LoadBitmap(Group->Assets, ID, true);
        Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
        
    }
	if (Bitmap) {
		PushBitmap(Group, Bitmap, Height, Offset, Color, CAlign);
	}
	else {
        Assert(!Group->RenderInBackground);
		LoadBitmap(Group->Assets, ID, false);
        ++Group->MissingResourceCount;
	}
}

inline loaded_font*
PushFont(render_group* Group, font_id ID) {
	loaded_font* Font = GetFont(Group->Assets, ID, Group->GenerationID);
	if (Font) {
		//PushBitmap(Group, Font, Height, Offset, Color);
	}
	else {
        Assert(!Group->RenderInBackground);
		LoadFont(Group->Assets, ID, false);
        ++Group->MissingResourceCount;
	}
    return(Font);
}


inline void
PushRect(render_group* Group, v3 Offset, v2 Dim, v4 Color) {
	v3 P = Offset - V3(0.5f * Dim, 0);
	entity_basis_p_result Basis = GetRenderEntityBasisP(&Group->Transform, P);
	if (Basis.Valid) {
		render_entry_rectangle* Entry = PushRenderElement(Group, render_entry_rectangle);
		if (Entry) {
			Entry->P = Basis.P;
			Entry->Color = Color;
			Entry->Dim = Basis.Scale * Dim;
		}
	}
}

inline void
PushRect(render_group* Group, rectangle2 Rect, r32 Z, v4 Color) {
	PushRect(Group, V3(GetCenter(Rect), Z), GetDim(Rect), Color);
}

inline void
PushRectOutline(render_group* Group, v3 Offset, v2 Dim, v4 Color, r32 Thickness = 0.1f) {
    
	
	// top - bottom
	PushRect(Group, Offset - v3{ 0, 0.5f * Dim.y, 0 }, v2{ Dim.x, Thickness }, Color);
	PushRect(Group, Offset + v3{ 0, 0.5f * Dim.y, 0 }, v2{ Dim.x, Thickness }, Color);
    
	PushRect(Group, Offset - v3{ 0.5f * Dim.x, 0, 0 }, v2{ Thickness, Dim.y }, Color);
	PushRect(Group, Offset + v3{ 0.5f * Dim.x, 0, 0 }, v2{ Thickness, Dim.y }, Color);
}

inline void
Clear(render_group* Group, v4 Color) {
    
	render_entry_clear* Piece = PushRenderElement(Group, render_entry_clear);
	if (Piece) {
		Piece->Color = Color;
	}
}

render_entry_coordinate_system*
CoordinateSystem(render_group* Group, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture, loaded_bitmap* NormalMap,
                 environment_map* Top, environment_map* Middle, environment_map* Bottom) {
	render_entry_coordinate_system* Entry = PushRenderElement(Group, render_entry_coordinate_system);
	if (Entry) {
		Entry->AxisX = AxisX;
		Entry->AxisY = AxisY;
		Entry->Color = Color;
		Entry->Origin = Origin;
		Entry->Texture = Texture;
		Entry->NormalMap = NormalMap;
		Entry->Top = Top;
		Entry->Middle = Middle;
		Entry->Bottom = Bottom;
	}
	return(Entry);
}

inline void
Perspective(render_group* RenderGroup, u32 PixelWidth, u32 PixelHeight, r32 FocalLength, r32 DistanceAboveTarget) {
	r32 WidthOfMonitor = 0.635f;
	r32 MetersToPixels = (r32)PixelWidth * WidthOfMonitor;
	r32 PixelsToMeters = SafeRatio1(1.0f, MetersToPixels);
	RenderGroup->Transform.FocalLength = FocalLength;
	RenderGroup->Transform.DistanceAboveTarget = DistanceAboveTarget;
	RenderGroup->Transform.MetersToPixels = MetersToPixels;
	RenderGroup->MonitorHalfDimInMeters = v2{
		0.5f * PixelsToMeters * PixelWidth,
		0.5f * PixelsToMeters * PixelHeight
	};
	RenderGroup->Transform.ScreenCenter = v2{ 0.5f * PixelWidth, 0.5f * PixelHeight };
	RenderGroup->Transform.Orthographic = false;
    RenderGroup->Transform.OffsetP = v3{0, 0, 0};
    RenderGroup->Transform.Scale = 1.0f;
}


inline void
Orthographic(render_group* RenderGroup, u32 PixelWidth, u32 PixelHeight, r32 MetersToPixels) {
	r32 PixelsToMeters = SafeRatio1(1.0f, MetersToPixels);
	RenderGroup->Transform.FocalLength = 1.0f;
	RenderGroup->Transform.DistanceAboveTarget = 1.0f;
	RenderGroup->Transform.MetersToPixels = MetersToPixels;
	RenderGroup->MonitorHalfDimInMeters = v2{
		0.5f * PixelsToMeters * PixelWidth,
		0.5f * PixelsToMeters * PixelHeight
	};
	RenderGroup->Transform.ScreenCenter = v2{ 0.5f * PixelWidth, 0.5f * PixelHeight };
	RenderGroup->Transform.Orthographic = true;
    RenderGroup->Transform.OffsetP = v3{0, 0, 0};
    RenderGroup->Transform.Scale = 1.0f;
}

inline b32
AllResoucePresent(render_group* Group) {
	b32 Result = (Group->MissingResourceCount == 0);
	return(Result);
}
inline v3
Unproject(render_group* Group, v2 PixelsXY) {
    v2 UnprojectedXY = {};
    render_transform *Transform = &Group->Transform;
    
    if (Transform->Orthographic) {
        UnprojectedXY = (1.0f / Transform->MetersToPixels) * (PixelsXY - Transform->ScreenCenter);
    } else {
        v2 dP = (PixelsXY - Transform->ScreenCenter) * (1.0f / Transform->MetersToPixels);
        UnprojectedXY = ((Transform->DistanceAboveTarget - Transform->OffsetP.z) / Transform->FocalLength) * dP;
    }
    v3 Result = V3(UnprojectedXY, Transform->OffsetP.z);
    Result -= Transform->OffsetP;
    return(Result);
}

inline render_group
BeginRenderGroup(game_assets* Assets, game_render_commands *Commands, u32 GenerationID, b32 RenderInBackground) {
	
    render_group Result = {};
	Result.Transform.OffsetP = v3{ 0, 0, 0 };
	Result.Transform.Scale = 1.0f;
    Result.GenerationID = GenerationID;
    
    Result.Commands = Commands;
    
    Result.Assets = Assets;
	Result.GlobalAlpha = 1.0f;
	Result.MissingResourceCount = 0;
    Result.RenderInBackground = RenderInBackground;
    
    return(Result);
}
inline void
EndRenderGroup(render_group *RenderGroup) {
    
}