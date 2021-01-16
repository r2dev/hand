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
    IGNORED_TIMED_FUNCTION();
    Assert(Group->InsideRender);
	void* Result = 0;
	Size += sizeof(render_group_entry_header);
	if (Group->PushBufferSize + Size < Group->MaxPushBufferSize) {
		render_group_entry_header* Header = (render_group_entry_header*)(Group->PushBufferBase + Group->PushBufferSize);
		Header->Type = Type;
		Result = Header + 1;
		Group->PushBufferSize += Size;
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

render_group*
AllocateRenderGroup(game_assets* Assets, memory_arena* Arena, u32 MaxPushBufferSize, b32 RenderInBackground) {
	render_group* Result = PushStruct(Arena, render_group);
    
	if (MaxPushBufferSize == 0) {
		MaxPushBufferSize = (u32)GetArenaSizeRemaining(Arena);
	}
	Result->Transform.OffsetP = v3{ 0, 0, 0 };
	Result->Transform.Scale = 1.0f;
    Result->GenerationID = 0;
    
	Result->PushBufferBase = (u8*)PushSize(Arena, MaxPushBufferSize);
	Result->MaxPushBufferSize = MaxPushBufferSize;
	Result->Assets = Assets;
	Result->PushBufferSize = 0;
	Result->GlobalAlpha = 1.0f;
	Result->MissingResourceCount = 0;
    Result->RenderInBackground = RenderInBackground;
    Result->InsideRender = false;
    return(Result);
}

internal void
BeginRender(render_group* Group) {
    if (Group) {
        Assert(!Group->InsideRender);
        Group->InsideRender = true;
        Group->GenerationID = BeginGeneration(Group->Assets);
    }
    
}

internal void
EndRender(render_group* Group) {
    if (Group) {
        Assert(Group->InsideRender);
        Group->InsideRender = false;
        
        EndGeneration(Group->Assets, Group->GenerationID);
        Group->GenerationID = 0;
        Group->PushBufferSize = 0;
    }
}

internal void
DrawBitmap(loaded_bitmap* Buffer, loaded_bitmap* Bitmap,
           r32 RealX, r32 RealY, r32 CAlpha = 1.0f) {
    IGNORED_TIMED_FUNCTION();
	s32 MinX = RoundReal32ToInt32(RealX);
	s32 MinY = RoundReal32ToInt32(RealY);
	s32 MaxX = MinX + Bitmap->Width;
	s32 MaxY = MinY + Bitmap->Height;
    
	s32 SourceOfferX = 0;
	if (MinX < 0) {
		SourceOfferX = -MinX;
		MinX = 0;
	}
	s32 SourceOfferY = 0;
	if (MinY < 0) {
		SourceOfferY = -MinY;
		MinY = 0;
	}
	if (MaxY > Buffer->Height) {
		MaxY = Buffer->Height;
	}
	if (MaxX > Buffer->Width) {
		MaxX = Buffer->Width;
	}
    
	u8* SourceRow = (u8*)Bitmap->Memory + SourceOfferY * Bitmap->Pitch + BITMAP_BYTE_PER_PIXEL * SourceOfferX;
	u8* DestRow = ((u8*)Buffer->Memory + MinX * BITMAP_BYTE_PER_PIXEL + MinY * Buffer->Pitch);
    
	for (int Y = MinY; Y < MaxY; ++Y) {
		u32* Dest = (u32*)DestRow;
		u32* Source = (u32*)SourceRow;
		for (int X = MinX; X < MaxX; ++X) {
#if 0
			r32 SA = ((r32)((*Source >> 24) & 0xFF));
			r32 RSA = (SA / 255.0f) * CAlpha;
			r32 SR = CAlpha * (r32)((*Source >> 16) & 0xFF);
			r32 SG = CAlpha * (r32)((*Source >> 8) & 0xFF);
			r32 SB = CAlpha * (r32)((*Source >> 0) & 0xFF);
            
			r32 DA = (r32)((*Dest >> 24) & 0xFF);
			r32 DR = (r32)((*Dest >> 16) & 0xFF);
			r32 DG = (r32)((*Dest >> 8) & 0xFF);
			r32 DB = (r32)((*Dest >> 0) & 0xFF);
			r32 RDA = (DA / 255.0f);
            
			r32 InvRSA = (1.0f - RSA);
            
			r32 A = 255.0f * (RSA + RDA - RSA * RDA);
			r32 R = InvRSA * DR + SR;
			r32 G = InvRSA * DG + SG;
			r32 B = InvRSA * DB + SB;
            
			*Dest = (u32)(A + 0.5f) << 24 | (u32)(R + 0.5f) << 16 | (u32)(G + 0.5f) << 8 | (u32)(B + 0.5f) << 0;
#else
			v4 Texel = {
				(r32)((*Source >> 16) & 0xFF),
				(r32)((*Source >> 8) & 0xFF),
				(r32)((*Source >> 0) & 0xFF),
				(r32)((*Source >> 24) & 0xFF)
			};
            
			Texel = SRGBToLinear1(Texel);
			Texel *= CAlpha;
			v4 D = {
				(r32)((*Dest >> 16) & 0xFF),
				(r32)((*Dest >> 8) & 0xFF),
				(r32)((*Dest >> 0) & 0xFF),
				(r32)((*Dest >> 24) & 0xFF)
			};
			D = SRGBToLinear1(D);
			v4 Result = (1.0f - Texel.a) * D + Texel;
			Result = Linear1ToSRGB(Result);
			*Dest = (u32)(Result.a + 0.5f) << 24 |
				(u32)(Result.r + 0.5f) << 16 |
				(u32)(Result.g + 0.5f) << 8 |
				(u32)(Result.b + 0.5f) << 0;
#endif
			++Dest;
			++Source;
		}
		DestRow += Buffer->Pitch;
		SourceRow += Bitmap->Pitch;
	}
}



inline v4
Unpack4x8(u32 Packed) {
	v4 Result = v4{
		(r32)((Packed >> 16) & 0xFF),
		(r32)((Packed >> 8) & 0xFF),
		(r32)((Packed >> 0) & 0xFF),
		(r32)((Packed >> 24) & 0xFF)
	};
	return(Result);
}

struct bilinear_sample {
	u32 A, B, C, D;
};

inline bilinear_sample
BilinearSample(loaded_bitmap* Texture, s32 X, s32 Y) {
	bilinear_sample Result = {};
	u8* Color8 = (u8*)Texture->Memory + Texture->Pitch * Y + X * sizeof(u32);
	Result.A = *(u32*)Color8;
	Result.B = *(u32*)(Color8 + sizeof(u32));
    
	Result.C = *(u32*)(Color8 + Texture->Pitch);
	Result.D = *(u32*)(Color8 + Texture->Pitch + sizeof(u32));
	return(Result);
}

inline v4
SRGBBilinearBlend(bilinear_sample TexelSample, r32 fx, r32 fy) {
	v4 Result = {};
	v4 TexelA = Unpack4x8(TexelSample.A);
	v4 TexelB = Unpack4x8(TexelSample.B);
	v4 TexelC = Unpack4x8(TexelSample.C);
	v4 TexelD = Unpack4x8(TexelSample.D);
	TexelA = SRGBToLinear1(TexelA);
	TexelB = SRGBToLinear1(TexelB);
	TexelC = SRGBToLinear1(TexelC);
	TexelD = SRGBToLinear1(TexelD);
	Result = Lerp(Lerp(TexelA, fx, TexelB), fy, Lerp(TexelC, fx, TexelD));
	return(Result);
}


inline v4
UnscaleAndBiasNormal(v4 Normal)
{
	v4 Result;
    
	r32 Inv255 = 1.0f / 255.0f;
    
	Result.x = -1.0f + 2.0f * (Inv255 * Normal.x);
	Result.y = -1.0f + 2.0f * (Inv255 * Normal.y);
	Result.z = -1.0f + 2.0f * (Inv255 * Normal.z);
    
	Result.w = Inv255 * Normal.w;
    
	return(Result);
}

internal v3
SampleEnvironmentMap(v2 ScreenUV, v3 SampleDirection, r32 Roughness, environment_map* Map, r32 DistanceFromMapInZ) {
    
	u32 LODIndex = (u32)(Roughness * (r32)(ArrayCount(Map->LOD) - 1) + 0.5f);
	loaded_bitmap* LOD = Map->LOD + LODIndex;
    
	r32 UVsPerMeter = 0.1f;
	r32 C = (UVsPerMeter * DistanceFromMapInZ) / SampleDirection.y;
	v2 Offset = C * v2{ SampleDirection.x, SampleDirection.z };
    
	v2 UV = ScreenUV + Offset;
	UV.x = Clamp01(UV.x);
	UV.y = Clamp01(UV.y);
    
	r32 tX = UV.x * (r32)(LOD->Width - 2);
	r32 tY = UV.y * (r32)(LOD->Height - 2);
    
	s32 X = (s32)tX;
	s32 Y = (s32)tY;
    
	r32 fX = tX - (r32)X;
	r32 fY = tY - (r32)Y;
    
    Assert(X >= 0 && X < LOD->Width);
    Assert(Y >= 0 && Y < LOD->Height);
    DEBUG_IF(Renderer_ShowLightning_SampleSource)
    {
        u8 *TexelPtr = ((u8*)LOD->Memory + Y*LOD->Pitch + X *sizeof(u32));
        *(u32 *)TexelPtr = 0xFFFFFFFF;
    }
	bilinear_sample Sample = BilinearSample(LOD, X, Y);
	v3 Result = SRGBBilinearBlend(Sample, fX, fY).xyz;
    
	return Result;
}

internal void
DrawRectangle1(loaded_bitmap* Buffer, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture, loaded_bitmap* NormalMap,
               environment_map* Top, environment_map* Middle, environment_map* Bottom, r32 PixelsToMeters) {
    // IGNORED_TIMED_FUNCTION();
#if 0
	u32 Color32 = (u32)(RoundReal32ToUInt32(Color.r * 255) << 16 |
                        RoundReal32ToUInt32(Color.g * 255) << 8 |
                        RoundReal32ToUInt32(Color.b * 255) << 0);
#endif
    
	r32 AxisXLength = Length(AxisX);
	r32 AxisYLength = Length(AxisY);
    
	v2 NxAxis = (AxisYLength / AxisXLength) * AxisX;
	v2 NyAxis = (AxisXLength / AxisYLength) * AxisY;
    
	r32 NzScale = 0.5f * (AxisXLength + AxisYLength);
    
	r32 InvAxisXLengthSq = 1.0f / LengthSq(AxisX);
	r32 InvAxisYLengthSq = 1.0f / LengthSq(AxisY);
	u32 Color32 = (RoundReal32ToUInt32(Color.a * 255.0f) << 24 |
                   RoundReal32ToUInt32(Color.r * 255.0f) << 16 |
                   RoundReal32ToUInt32(Color.g * 255.0f) << 8 |
                   RoundReal32ToUInt32(Color.b * 255.0f) << 0);
	s32 WidthMax = Buffer->Width - 1;
	s32 HeightMax = Buffer->Height - 1;
    
	r32 invWidthMax = (r32)1.0f / WidthMax;
	r32 invHeightMax = (r32)1.0f / HeightMax;
	s32 MinX = WidthMax;
	s32 MaxX = 0;
	s32 MinY = HeightMax;
	s32 MaxY = 0;
    
	v2 P[4] = { Origin, Origin + AxisX, Origin + AxisX + AxisY, Origin + AxisY };
    
	r32 OriginZ = 0.0f;
	r32 OriginY = (Origin + 0.5f * AxisX + 0.5f * AxisY).y;
	r32 FixedCastY = invHeightMax * OriginY;
    
	for (s32 PIndex = 0; PIndex < ArrayCount(P); ++PIndex) {
		v2 Cur = P[PIndex];
		s32 FloorX = FloorReal32ToInt32(Cur.x);
		s32 FloorY = FloorReal32ToInt32(Cur.y);
		s32 CeilX = CeilReal32ToInt32(Cur.x);
		s32 CeilY = CeilReal32ToInt32(Cur.y);
        
		if (Cur.x > MaxX) { MaxX = CeilX; }
		if (Cur.y > MaxY) { MaxY = CeilY; }
        
		if (FloorX < MinX) { MinX = FloorX; }
		if (FloorY < MinY) { MinY = FloorY; }
	}
	if (MinX < 0) {
		MinX = 0;
	}
    
	if (MinY < 0) {
		MinY = 0;
	}
	if (MaxY > HeightMax) {
		MaxY = HeightMax;
	}
	if (MaxX > WidthMax) {
		MaxX = WidthMax;
	}
    
	u8* Row = ((u8*)Buffer->Memory + MinX * BITMAP_BYTE_PER_PIXEL + MinY * Buffer->Pitch);
    TIMED_BLOCK(PixelFill, (MaxX - MinX + 1) * (MaxY - MinY + 1));
	for (int Y = MinY; Y < MaxY; ++Y) {
		u32* Pixel = (u32*)Row;
		for (int X = MinX; X < MaxX; ++X) {
			v2 tempP = V2i(X, Y);
			v2 D = tempP - Origin;
            
			r32 Edge0 = Inner(D, -Perp(AxisX));
			r32 Edge1 = Inner(D - AxisX, -Perp(AxisY));
			r32 Edge2 = Inner(D - AxisY - AxisX, Perp(AxisX));
			r32 Edge3 = Inner(D - AxisY, Perp(AxisY));
            
			if (Edge0 < 0 && Edge1 < 0 && Edge2 < 0 && Edge3 < 0) {
                
				v2 ScreenSpaceUV = { invWidthMax * (r32)X, FixedCastY };
				r32 ZDiff = PixelsToMeters * ((r32)Y - OriginY);
                
				r32 U = InvAxisXLengthSq * Inner(D, AxisX);
				r32 V = InvAxisYLengthSq * Inner(D, AxisY);
                
				r32 tX = U * (r32)(Texture->Width - 2);
				r32 tY = V * (r32)(Texture->Height - 2);
                
				s32 intX = (s32)tX;
				s32 intY = (s32)tY;
                
                
				r32 fX = tX - (r32)intX;
				r32 fY = tY - (r32)intY;
                
				bilinear_sample TexelSample = BilinearSample(Texture, intX, intY);
				v4 Texel = SRGBBilinearBlend(TexelSample, fX, fY);
                
				if (NormalMap) {
					bilinear_sample NormalSample = BilinearSample(NormalMap, intX, intY);
					v4 NormalA = Unpack4x8(NormalSample.A);
					v4 NormalB = Unpack4x8(NormalSample.B);
					v4 NormalC = Unpack4x8(NormalSample.C);
					v4 NormalD = Unpack4x8(NormalSample.D);
                    
					v4 Normal = Lerp(Lerp(NormalA, fX, NormalB),
                                     fY,
                                     Lerp(NormalC, fX, NormalD));
					Normal = UnscaleAndBiasNormal(Normal);
					Normal.xy = Normal.x * NxAxis + Normal.y * NyAxis;
					Normal.z *= NzScale;
					Normal.xyz = Normalize(Normal.xyz);
                    
					v3 EyeDirection = v3{ 0, 0, 1.0f };
					r32 projectionLength = 2.0f * Inner(EyeDirection, Normal.xyz);
					v3 BounceDirection = -EyeDirection + projectionLength * Normal.xyz;
					BounceDirection.z = -BounceDirection.z;
                    
					environment_map* FarMap = 0;
					r32 Pz = OriginZ + ZDiff;
					r32 tEnvMap = BounceDirection.y;
					r32 tFarMap = 0.0f;
					if (tEnvMap < -0.5f) {
						FarMap = Bottom;
						tFarMap = -1 - 2.0f * tEnvMap;
					}
					else if (tEnvMap > 0.5f) {
						FarMap = Top;
						tFarMap = 2 * (tEnvMap - 0.5f);
					}
					tFarMap *= tFarMap;
					tFarMap *= tFarMap;
                    
					v3 LightColor = { 0, 0, 0 };
					if (FarMap) {
						r32 DistanceFromMapInZ = FarMap->Pz - Pz;
						v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, BounceDirection, Normal.w, FarMap, DistanceFromMapInZ);
						LightColor = Lerp(LightColor, tFarMap, FarMapColor);
					}
                    
					Texel.rgb += Texel.a * LightColor;
				}
                
				Texel = Hadamard(Texel, Color);
				Texel.r = Clamp01(Texel.r);
				Texel.g = Clamp01(Texel.g);
				Texel.b = Clamp01(Texel.b);
				v4 Dest = Unpack4x8(*Pixel);
				Dest = SRGBToLinear1(Dest);
				v4 Blended = (1.0f - Texel.a) * Dest + Texel;
                
				Blended = Linear1ToSRGB(Blended);
                
				*Pixel = (u32)(Blended.a + 0.5f) << 24 |
					(u32)(Blended.r + 0.5f) << 16 |
					(u32)(Blended.g + 0.5f) << 8 |
					(u32)(Blended.b + 0.5f) << 0;
                
			}
			*Pixel++;
            
		}
		Row += Buffer->Pitch;
	}
    
}


internal void
DrawRectangle(loaded_bitmap* Buffer,
              v2 vMin, v2 vMax,
              v4 Color, rectangle2i ClipRect, b32 Even) {
    IGNORED_TIMED_FUNCTION();
	r32 R = Color.r;
	r32 G = Color.g;
	r32 B = Color.b;
	r32 A = Color.a;
	
	rectangle2i FillRect;
	FillRect.MinX = RoundReal32ToInt32(vMin.x);
	FillRect.MinY = RoundReal32ToInt32(vMin.y);
	FillRect.MaxX = RoundReal32ToInt32(vMax.x);
	FillRect.MaxY = RoundReal32ToInt32(vMax.y);
	
	FillRect = Intersect(FillRect, ClipRect);
	
	if (!Even == (FillRect.MinY & 1)) {
		FillRect.MinY += 1;
	}
    
	// BIT PATTERN AA RR GG BB
	u32 Color32 = (u32)
		(RoundReal32ToUInt32(A * 255) << 24 |
         RoundReal32ToUInt32(R * 255) << 16 |
         RoundReal32ToUInt32(G * 255) << 8 |
         RoundReal32ToUInt32(B * 255) << 0);
    
	u8* Row = ((u8*)Buffer->Memory + FillRect.MinX * BITMAP_BYTE_PER_PIXEL + FillRect.MinY * Buffer->Pitch);
    
	for (int Y = FillRect.MinY; Y < FillRect.MaxY; Y += 2) {
		u32* Pixel = (u32*)Row;
		for (int X = FillRect.MinX; X < FillRect.MaxX; ++X) {
			*Pixel++ = Color32;
		}
		Row += 2 * Buffer->Pitch;
	}
}


internal void
RenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget, rectangle2i ClipRect, b32 Even) {
    IGNORED_TIMED_FUNCTION();
    v2 ScreenDim = { (r32)OutputTarget->Width, (r32)OutputTarget->Height };
	for (u32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize;) {
		render_group_entry_header* Header = (render_group_entry_header*)(RenderGroup->PushBufferBase + BaseAddress);
		BaseAddress += sizeof(*Header);
		void* Data = Header + 1;
        
		//todo null
		r32 NullPixelsToMeters = 1.0f;
        
		switch (Header->Type) {
            case RenderGroupEntryType_render_entry_clear: {
                render_entry_clear* Entry = (render_entry_clear*)Data;
                
                DrawRectangle(OutputTarget, v2{ 0, 0 }, v2{ (r32)OutputTarget->Width, (r32)OutputTarget->Height }, Entry->Color, ClipRect, Even);
                BaseAddress += sizeof(*Entry);
            } break;
            
            case RenderGroupEntryType_render_entry_rectangle: {
                render_entry_rectangle* Entry = (render_entry_rectangle*)Data;
                
                DrawRectangle(OutputTarget, Entry->P, Entry->P + Entry->Dim, Entry->Color, ClipRect, Even);
                
                BaseAddress += sizeof(*Entry);
            } break;
            case RenderGroupEntryType_render_entry_bitmap: {
                render_entry_bitmap* Entry = (render_entry_bitmap*)Data;
                v2 XAxis = {1, 0};
                v2 YAxis = {0, 1};
                DrawRectangle2(OutputTarget, Entry->P, Entry->Size.x * XAxis, Entry->Size.y * YAxis, Entry->Color, Entry->Bitmap, NullPixelsToMeters, ClipRect, Even);
				
                BaseAddress += sizeof(*Entry);
                
            } break;
            case RenderGroupEntryType_render_entry_coordinate_system: {
                render_entry_coordinate_system* Entry = (render_entry_coordinate_system*)Data;
#if 0
                v2 P = Entry->Origin;
                
                DrawRectangle1(OutputTarget, P, Entry->AxisX, Entry->AxisY, Entry->Color, Entry->Texture,
                               Entry->NormalMap, Entry->Top, Entry->Middle, Entry->Bottom, PixelsToMeters);
                v2 Dim = { 2, 2 };
                v4 Color = { 1.0f, 1.0f, 0.0f, 1.0f };
                DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);
                
                P = Entry->Origin + Entry->AxisX;
                DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);
                
                P = Entry->Origin + Entry->AxisY;
                DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);
                P = Entry->Origin + Entry->AxisY + Entry->AxisX;
                
                DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);
#endif
                BaseAddress += sizeof(*Entry);
            } break;
			InvalidDefaultCase;
		}
	}
}

struct tile_render_work {
	render_group* RenderGroup;
	loaded_bitmap* OutputTarget;
	rectangle2i ClipRect;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(DoTileRenderWork) {
	tile_render_work* Work = (tile_render_work*)Data;
    
	RenderGroupToOutput(Work->RenderGroup, Work->OutputTarget, Work->ClipRect, false);
	RenderGroupToOutput(Work->RenderGroup, Work->OutputTarget, Work->ClipRect, true);
}


internal void 
TiledRenderGroupToOutput(platform_work_queue* Queue, render_group* RenderGroup, loaded_bitmap* OutputTarget) {
    IGNORED_TIMED_FUNCTION();
    Assert(RenderGroup->InsideRender);
	u32 const TileCountX = 4;
	u32 const TileCountY = 4;
    
    
	Assert(((uintptr)OutputTarget->Memory & 15) == 0);
	u32 TileWidth = OutputTarget->Width / TileCountX;
	u32 TileHeight = OutputTarget->Height / TileCountY;
	TileWidth = ((TileWidth + 3) / 4) * 4;
#if 1
	tile_render_work Works[TileCountX * TileCountY];
	for (u32 Y = 0; Y < TileCountY; Y++) {
		for (u32 X = 0; X < TileCountX; X++) {
            
			
			rectangle2i ClipRect;
			ClipRect.MinX = X * TileWidth;
			ClipRect.MinY = Y * TileHeight;
			
			ClipRect.MaxY = (Y + 1) * TileHeight;
			ClipRect.MaxX = (X + 1) * TileWidth;
            
            
			if (X + 1 == TileCountX) {
				ClipRect.MaxX = OutputTarget->Width;
			}
			if (Y + 1 == TileCountY) {
				ClipRect.MaxY = OutputTarget->Height;
			}
			
			tile_render_work *Work = Works + (Y * 4 + X);
			Work->ClipRect = ClipRect;
			Work->OutputTarget = OutputTarget;
			Work->RenderGroup = RenderGroup;
            
			Platform.AddEntry(Queue, DoTileRenderWork, Work);
		}
	}
	Platform.CompleteAllWork(Queue);
#else
	rectangle2i Cilp = { 0, 0, OutputTarget->Width, OutputTarget->Height };
	RenderGroupToOutput(RenderGroup, OutputTarget, Cilp, false);
#endif
}

internal void
RenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget) {
    IGNORED_TIMED_FUNCTION();
    Assert(RenderGroup->InsideRender);
	rectangle2i ClipRect;
	ClipRect.MinX = 0;
	ClipRect.MinY = 0;
	ClipRect.MaxY = OutputTarget->Height;
	ClipRect.MaxX = OutputTarget->Width;
    
	tile_render_work Work;
	Work.ClipRect = ClipRect;
	Work.RenderGroup = RenderGroup;
	Work.OutputTarget = OutputTarget;
	DoTileRenderWork(0, &Work);
}


inline void
RenderToOutput(platform_work_queue* Queue, render_group* RenderGroup, loaded_bitmap* OutputTarget) {
    if (1) {
        Platform.RenderToOpenGL(RenderGroup, OutputTarget);
    } else {
        TiledRenderGroupToOutput(Queue, RenderGroup, OutputTarget);
    }
    
}