
#include "handmade.h"


struct entity_basis_p_result {
	v2 P;
	real32 Scale;
	bool32 Valid;
};

inline entity_basis_p_result
GetRenderEntityBasisP(render_transform* Transform, v3 OriginP) {
	entity_basis_p_result Result = {};

	v3 P = V3(OriginP.xy, 0.0f) + Transform->OffsetP;

	real32 DistancePz = Transform->DistanceAboveTarget - P.z;
	v3 RawXY = V3(P.xy, 1.0f);
	real32 NearClipPlane = 0.2f;

	if (DistancePz > NearClipPlane) {
		v3 ProjectedXY = 1.0f / DistancePz * Transform->FocalLength * RawXY;
		Result.Scale = Transform->MetersToPixels * ProjectedXY.z;
		v2 EntityGround = Transform->ScreenCenter + Transform->MetersToPixels * ProjectedXY.xy;
		Result.P = EntityGround;
		Result.Valid = true;
	}

	return(Result);
}

#define PushRenderElement(Group, type)(type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type)


inline void*
PushRenderElement_(render_group* Group, uint32 Size, render_group_entry_type Type) {
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

inline void
PushBitmap(render_group* Group, loaded_bitmap* Bitmap, real32 Height, v3 Offset, v4 Color = v4{ 1.0f, 1.0, 1.0f, 1.0f }) {
	v2 Size = v2{ Height * Bitmap->WidthOverHeight, Height };
	v2 Align = Hadamard(Size, Bitmap->AlignPercentage);
	v3 P = Offset - V3(Align, 0);
	entity_basis_p_result Basis = GetRenderEntityBasisP(&Group->Transform, P);
	if (Basis.Valid) {
		render_entry_bitmap* Entry = PushRenderElement(Group, render_entry_bitmap);
		if (Entry) {
			Entry->P = Basis.P;
			Entry->Bitmap = Bitmap;
			Entry->Color = Group->GlobalAlpha * Color;
			Entry->Size = Basis.Scale * Size;
		}
	}
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
PushRectOutline(render_group* Group, v3 Offset, v2 Dim, v4 Color) {

	real32 Thickness = 0.1f;
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

render_group*
AllocateRenderGroup(memory_arena* Arena, uint32 MaxPushBufferSize, uint32 ResolutionPixelX, uint32 ResolutionPixelY) {
	render_group* Result = PushStruct(Arena, render_group);
	real32 WidthOfMonitor = 0.635f;
	real32 MetersToPixels = (real32)ResolutionPixelX * WidthOfMonitor;
	real32 PixelsToMeters = SafeRatio1(1.0f, MetersToPixels);
	Result->Transform.MetersToPixels = MetersToPixels;
	Result->Transform.ScreenCenter = v2{ 0.5f * ResolutionPixelX, 0.5f * ResolutionPixelY };
	Result->Transform.FocalLength = 0.6f;
	Result->Transform.DistanceAboveTarget = 9.0f;
	Result->Transform.OffsetP = v3{ 0, 0, 0 };
	Result->Transform.Scale = 1.0f;

	Result->PushBufferBase = (uint8*)PushSize(Arena, MaxPushBufferSize);
	Result->MaxPushBufferSize = MaxPushBufferSize;

	Result->PushBufferSize = 0;
	Result->GlobalAlpha = 1.0f;

	Result->MonitorHalfDimInMeters = v2{
		0.5f * PixelsToMeters * ResolutionPixelX,
		0.5f * PixelsToMeters * ResolutionPixelY
	};
	
	return(Result);
}

internal v4
SRGBToLinear1(v4 C) {
	v4 Result = {};
	real32 Inv255C = 1.0f / 255.0f;
	Result.r = Square(Inv255C * C.r);
	Result.g = Square(Inv255C * C.g);
	Result.b = Square(Inv255C * C.b);
	Result.a = Inv255C * C.a;
	return(Result);
}


internal v4
Linear1ToSRGB(v4 C) {
	v4 Result = {};
	real32 One255C = 255.0f;
	Result.r = One255C * SquareRoot(C.r);
	Result.g = One255C * SquareRoot(C.g);
	Result.b = One255C * SquareRoot(C.b);
	Result.a = One255C * C.a;
	return(Result);
}


internal void
DrawBitmap(loaded_bitmap* Buffer, loaded_bitmap* Bitmap,
	real32 RealX, real32 RealY, real32 CAlpha = 1.0f) {
	int32 MinX = RoundReal32ToInt32(RealX);
	int32 MinY = RoundReal32ToInt32(RealY);
	int32 MaxX = MinX + Bitmap->Width;
	int32 MaxY = MinY + Bitmap->Height;

	int32 SourceOfferX = 0;
	if (MinX < 0) {
		SourceOfferX = -MinX;
		MinX = 0;
	}
	int32 SourceOfferY = 0;
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

	uint8* SourceRow = (uint8*)Bitmap->Memory + SourceOfferY * Bitmap->Pitch + BITMAP_BYTE_PER_PIXEL * SourceOfferX;
	uint8* DestRow = ((uint8*)Buffer->Memory + MinX * BITMAP_BYTE_PER_PIXEL + MinY * Buffer->Pitch);

	for (int Y = MinY; Y < MaxY; ++Y) {
		uint32* Dest = (uint32*)DestRow;
		uint32* Source = (uint32*)SourceRow;
		for (int X = MinX; X < MaxX; ++X) {
#if 0
			real32 SA = ((real32)((*Source >> 24) & 0xFF));
			real32 RSA = (SA / 255.0f) * CAlpha;
			real32 SR = CAlpha * (real32)((*Source >> 16) & 0xFF);
			real32 SG = CAlpha * (real32)((*Source >> 8) & 0xFF);
			real32 SB = CAlpha * (real32)((*Source >> 0) & 0xFF);

			real32 DA = (real32)((*Dest >> 24) & 0xFF);
			real32 DR = (real32)((*Dest >> 16) & 0xFF);
			real32 DG = (real32)((*Dest >> 8) & 0xFF);
			real32 DB = (real32)((*Dest >> 0) & 0xFF);
			real32 RDA = (DA / 255.0f);

			real32 InvRSA = (1.0f - RSA);

			real32 A = 255.0f * (RSA + RDA - RSA * RDA);
			real32 R = InvRSA * DR + SR;
			real32 G = InvRSA * DG + SG;
			real32 B = InvRSA * DB + SB;

			*Dest = (uint32)(A + 0.5f) << 24 | (uint32)(R + 0.5f) << 16 | (uint32)(G + 0.5f) << 8 | (uint32)(B + 0.5f) << 0;
#else
			v4 Texel = {
				(real32)((*Source >> 16) & 0xFF),
				(real32)((*Source >> 8) & 0xFF),
				(real32)((*Source >> 0) & 0xFF),
				(real32)((*Source >> 24) & 0xFF)
			};

			Texel = SRGBToLinear1(Texel);
			Texel *= CAlpha;
			v4 D = {
				(real32)((*Dest >> 16) & 0xFF),
				(real32)((*Dest >> 8) & 0xFF),
				(real32)((*Dest >> 0) & 0xFF),
				(real32)((*Dest >> 24) & 0xFF)
			};
			D = SRGBToLinear1(D);
			v4 Result = (1.0f - Texel.a) * D + Texel;
			Result = Linear1ToSRGB(Result);
			*Dest = (uint32)(Result.a + 0.5f) << 24 |
				(uint32)(Result.r + 0.5f) << 16 |
				(uint32)(Result.g + 0.5f) << 8 |
				(uint32)(Result.b + 0.5f) << 0;
#endif
			++Dest;
			++Source;
		}
		DestRow += Buffer->Pitch;
		SourceRow += Bitmap->Pitch;
	}
}



inline v4
Unpack4x8(uint32 Packed) {
	v4 Result = v4{
		(real32)((Packed >> 16) & 0xFF),
		(real32)((Packed >> 8) & 0xFF),
		(real32)((Packed >> 0) & 0xFF),
		(real32)((Packed >> 24) & 0xFF)
	};
	return(Result);
}

struct bilinear_sample {
	uint32 A, B, C, D;
};

inline bilinear_sample
BilinearSample(loaded_bitmap* Texture, int32 X, int32 Y) {
	bilinear_sample Result = {};
	uint8* Color8 = (uint8*)Texture->Memory + Texture->Pitch * Y + X * sizeof(uint32);
	Result.A = *(uint32*)Color8;
	Result.B = *(uint32*)(Color8 + sizeof(uint32));

	Result.C = *(uint32*)(Color8 + Texture->Pitch);
	Result.D = *(uint32*)(Color8 + Texture->Pitch + sizeof(uint32));
	return(Result);
}

inline v4
SRGBBilinearBlend(bilinear_sample TexelSample, real32 fx, real32 fy) {
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

	real32 Inv255 = 1.0f / 255.0f;

	Result.x = -1.0f + 2.0f * (Inv255 * Normal.x);
	Result.y = -1.0f + 2.0f * (Inv255 * Normal.y);
	Result.z = -1.0f + 2.0f * (Inv255 * Normal.z);

	Result.w = Inv255 * Normal.w;

	return(Result);
}

internal v3
SampleEnvironmentMap(v2 ScreenUV, v3 SampleDirection, real32 Roughness, environment_map* Map, real32 DistanceFromMapInZ) {

	uint32 LODIndex = (uint32)(Roughness * (real32)(ArrayCount(Map->LOD) - 1) + 0.5f);
	loaded_bitmap* LOD = Map->LOD + LODIndex;

	real32 UVsPerMeter = 0.1f;
	real32 C = (UVsPerMeter * DistanceFromMapInZ) / SampleDirection.y;
	v2 Offset = C * v2{ SampleDirection.x, SampleDirection.z };

	v2 UV = ScreenUV + Offset;
	UV.x = Clamp01(UV.x);
	UV.y = Clamp01(UV.y);

	real32 tX = UV.x * (real32)(LOD->Width - 2);
	real32 tY = UV.y * (real32)(LOD->Height - 2);

	int32 X = (int32)tX;
	int32 Y = (int32)tY;

	real32 fX = tX - (real32)X;
	real32 fY = tY - (real32)Y;

	bilinear_sample Sample = BilinearSample(LOD, X, Y);
	v3 Result = SRGBBilinearBlend(Sample, fX, fY).xyz;

	return Result;
}

internal void
DrawRectangle1(loaded_bitmap* Buffer, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture, loaded_bitmap* NormalMap,
	environment_map* Top, environment_map* Middle, environment_map* Bottom, real32 PixelsToMeters) {
#if 0
	uint32 Color32 = (uint32)(RoundReal32ToUInt32(Color.r * 255) << 16 |
		RoundReal32ToUInt32(Color.g * 255) << 8 |
		RoundReal32ToUInt32(Color.b * 255) << 0);
#endif

	real32 AxisXLength = Length(AxisX);
	real32 AxisYLength = Length(AxisY);

	v2 NxAxis = (AxisYLength / AxisXLength) * AxisX;
	v2 NyAxis = (AxisXLength / AxisYLength) * AxisY;

	real32 NzScale = 0.5f * (AxisXLength + AxisYLength);

	real32 InvAxisXLengthSq = 1.0f / LengthSq(AxisX);
	real32 InvAxisYLengthSq = 1.0f / LengthSq(AxisY);
	uint32 Color32 = (RoundReal32ToUInt32(Color.a * 255.0f) << 24 |
		RoundReal32ToUInt32(Color.r * 255.0f) << 16 |
		RoundReal32ToUInt32(Color.g * 255.0f) << 8 |
		RoundReal32ToUInt32(Color.b * 255.0f) << 0);
	int32 WidthMax = Buffer->Width - 1;
	int32 HeightMax = Buffer->Height - 1;

	real32 invWidthMax = (real32)1.0f / WidthMax;
	real32 invHeightMax = (real32)1.0f / HeightMax;
	int32 MinX = WidthMax;
	int32 MaxX = 0;
	int32 MinY = HeightMax;
	int32 MaxY = 0;

	v2 P[4] = { Origin, Origin + AxisX, Origin + AxisX + AxisY, Origin + AxisY };

	real32 OriginZ = 0.0f;
	real32 OriginY = (Origin + 0.5f * AxisX + 0.5f * AxisY).y;
	real32 FixedCastY = invHeightMax * OriginY;

	for (int32 PIndex = 0; PIndex < ArrayCount(P); ++PIndex) {
		v2 Cur = P[PIndex];
		int32 FloorX = FloorReal32ToInt32(Cur.x);
		int32 FloorY = FloorReal32ToInt32(Cur.y);
		int32 CeilX = CeilReal32ToInt32(Cur.x);
		int32 CeilY = CeilReal32ToInt32(Cur.y);

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

	uint8* Row = ((uint8*)Buffer->Memory + MinX * BITMAP_BYTE_PER_PIXEL + MinY * Buffer->Pitch);
	for (int Y = MinY; Y < MaxY; ++Y) {
		uint32* Pixel = (uint32*)Row;
		for (int X = MinX; X < MaxX; ++X) {
			v2 tempP = V2i(X, Y);
			v2 D = tempP - Origin;

			real32 Edge0 = Inner(D, -Perp(AxisX));
			real32 Edge1 = Inner(D - AxisX, -Perp(AxisY));
			real32 Edge2 = Inner(D - AxisY - AxisX, Perp(AxisX));
			real32 Edge3 = Inner(D - AxisY, Perp(AxisY));

			if (Edge0 < 0 && Edge1 < 0 && Edge2 < 0 && Edge3 < 0) {

				v2 ScreenSpaceUV = { invWidthMax * (real32)X, FixedCastY };
				real32 ZDiff = PixelsToMeters * ((real32)Y - OriginY);

				real32 U = InvAxisXLengthSq * Inner(D, AxisX);
				real32 V = InvAxisYLengthSq * Inner(D, AxisY);

				real32 tX = U * (real32)(Texture->Width - 2);
				real32 tY = V * (real32)(Texture->Height - 2);

				int32 intX = (int32)tX;
				int32 intY = (int32)tY;


				real32 fX = tX - (real32)intX;
				real32 fY = tY - (real32)intY;

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
					real32 projectionLength = 2.0f * Inner(EyeDirection, Normal.xyz);
					v3 BounceDirection = -EyeDirection + projectionLength * Normal.xyz;
					BounceDirection.z = -BounceDirection.z;

					environment_map* FarMap = 0;
					real32 Pz = OriginZ + ZDiff;
					real32 tEnvMap = BounceDirection.y;
					real32 tFarMap = 0.0f;
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
						real32 DistanceFromMapInZ = FarMap->Pz - Pz;
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

				*Pixel = (uint32)(Blended.a + 0.5f) << 24 |
					(uint32)(Blended.r + 0.5f) << 16 |
					(uint32)(Blended.g + 0.5f) << 8 |
					(uint32)(Blended.b + 0.5f) << 0;

			}
			*Pixel++;

		}
		Row += Buffer->Pitch;
	}
}



internal void
DrawRectangle2(loaded_bitmap* Buffer, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture, real32 PixelsToMeters, rectangle2i ClipRect, bool32 Even) {
	
	Assert(Texture->Memory);
	// premultiply alpha
	Color.rgb *= Color.a;
	
	real32 AxisXLength = Length(AxisX);
	real32 AxisYLength = Length(AxisY);

	v2 NxAxis = (AxisYLength / AxisXLength) * AxisX;
	v2 NyAxis = (AxisXLength / AxisYLength) * AxisY;

	real32 NzScale = 0.5f * (AxisXLength + AxisYLength);

	real32 InvAxisXLengthSq = 1.0f / LengthSq(AxisX);
	real32 InvAxisYLengthSq = 1.0f / LengthSq(AxisY);
	
	rectangle2i FillRect = InvertedInfinityRectangle();

	v2 P[4] = { Origin, Origin + AxisX, Origin + AxisX + AxisY, Origin + AxisY };

	for (int32 PIndex = 0; PIndex < ArrayCount(P); ++PIndex) {
		v2 TestP = P[PIndex];
		int32 FloorX = FloorReal32ToInt32(TestP.x);
		int32 CeilX = CeilReal32ToInt32(TestP.x) + 1;
		
		int32 FloorY = FloorReal32ToInt32(TestP.y);
		int32 CeilY = CeilReal32ToInt32(TestP.y) + 1;

		if (FillRect.MinX > FloorX) { FillRect.MinX = FloorX; }
		if (FillRect.MinY > FloorY) { FillRect.MinY = FloorY; }

		if (FillRect.MaxX < CeilX) { FillRect.MaxX = CeilX; }
		if (FillRect.MaxY < CeilY) { FillRect.MaxY = CeilY; }
	}
	
	FillRect = Intersect(ClipRect, FillRect);
	
	if (!Even == (FillRect.MinY & 1)) {
		FillRect.MinY += 1;
	}
	
	if (HasArea(FillRect)) {
		__m128i StartClipMask = _mm_set1_epi8(-1);
		__m128i StartClipMasks[] = {
			StartClipMask,
			_mm_slli_si128(StartClipMask, 1 * 4),
			_mm_slli_si128(StartClipMask, 2 * 4),
			_mm_slli_si128(StartClipMask, 3 * 4)
		};
		__m128i EndClipMask = _mm_set1_epi8(-1);
		__m128i EndClipMasks[] = {
			EndClipMask,
			_mm_srli_si128(EndClipMask, 3 * 4),
			_mm_srli_si128(EndClipMask, 2 * 4),
			_mm_srli_si128(EndClipMask, 1 * 4)
		};

		if (FillRect.MinX & 3) {
			StartClipMask = StartClipMasks[FillRect.MinX & 3];
			FillRect.MinX -= FillRect.MinX & 3;
		}

		if (FillRect.MaxX & 3) {
			EndClipMask = EndClipMasks[FillRect.MaxX & 3];
			FillRect.MaxX += (4 - FillRect.MaxX & 3);
		}

		void *TextureMemory = Texture->Memory;
		
		real32 Inv255C = 1.0f / 255.0f;
		real32 One255C = 255.0f;
		__m128 Inv255C_x4 = _mm_set_ps1(Inv255C);
		__m128 One255C_x4 = _mm_set_ps1(One255C);
		__m128 Four_x4 = _mm_set_ps1(4.0f);
		__m128 StepFour_x4 = _mm_set_ps(3.0f, 2.0f, 1.0f, 0.0f);
		__m128 Min_x4 = _mm_set_ps1((real32)FillRect.MinX);
		__m128i MaskFF00FF = _mm_set1_epi32(0x00FF00FF);
		__m128i MaskFFFF = _mm_set1_epi32(0xFFFF);
		__m128 One = _mm_set_ps1(1.0f);
		__m128 Zero = _mm_set_ps1(0.0f);
		__m128 ColorR = _mm_set_ps1(Color.r);
		__m128 ColorG = _mm_set_ps1(Color.g);
		__m128 ColorB = _mm_set_ps1(Color.b);
		__m128 ColorA = _mm_set_ps1(Color.a);
		__m128 OriginX_x4 = _mm_set_ps1(Origin.x);
		__m128 OriginY_x4 = _mm_set_ps1(Origin.y);

		__m128 XAxisx = _mm_set_ps1(AxisX.x);
		__m128 XAxisy = _mm_set_ps1(AxisX.y);
		__m128 YAxisx = _mm_set_ps1(AxisY.x);
		__m128 YAxisy = _mm_set_ps1(AxisY.y);

		__m128 WidthM2 = _mm_set1_ps((real32)(Texture->Width - 2));
		__m128 HeightM2 = _mm_set1_ps((real32)(Texture->Height - 2));

		__m128 InvAxisXLengthSq_x4 = _mm_set_ps1(InvAxisXLengthSq);
		__m128 InvAxisYLengthSq_x4 = _mm_set_ps1(InvAxisYLengthSq);

		__m128 NAxisXx = _mm_mul_ps(XAxisx, InvAxisXLengthSq_x4);
		__m128 NAxisXy = _mm_mul_ps(XAxisy, InvAxisXLengthSq_x4);
		__m128 NAxisYx = _mm_mul_ps(YAxisx, InvAxisYLengthSq_x4);
		__m128 NAxisYy = _mm_mul_ps(YAxisy, InvAxisYLengthSq_x4);
	

		__m128 MaxColorValue = _mm_set1_ps(255.0f * 255.0f);
		__m128i TexturePitch_x4 = _mm_set1_epi32(Texture->Pitch);
		__m128i MaskFF = _mm_set1_epi32(0xFF);
		int32 RowAdvance = 2 * Buffer->Pitch;
		
		uint8* Row = ((uint8*)Buffer->Memory + FillRect.MinX * BITMAP_BYTE_PER_PIXEL + FillRect.MinY * Buffer->Pitch);
		
		BEGIN_TIMED_BLOCK(FillPixel);
		for (int32 Y = FillRect.MinY; Y < FillRect.MaxY; Y += 2) {
			__m128 Y_x4 = _mm_set_ps1((real32)Y);
			__m128 Dy = _mm_sub_ps(Y_x4, OriginY_x4);
			
			__m128 PynX = _mm_mul_ps(Dy, NAxisXy);
			__m128 PynY = _mm_mul_ps(Dy, NAxisYy);
			
			__m128 tempPx = _mm_add_ps(Min_x4, StepFour_x4);
			tempPx = _mm_sub_ps(tempPx, OriginX_x4);
			
			__m128i ClipMask = StartClipMask;
			
			uint32* Pixel = (uint32*)Row;
			
			for (int32 X = FillRect.MinX; X < FillRect.MaxX; X += 4) {
				// load
				__m128i OriginalDest = _mm_load_si128((__m128i*)Pixel);
				__m128 U = _mm_add_ps(_mm_mul_ps(tempPx, NAxisXx), PynX);
				__m128 V = _mm_add_ps(_mm_mul_ps(tempPx, NAxisYx), PynY);
				
				// WriteMask ClipMask
				__m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, Zero), _mm_cmple_ps(U, One)), _mm_and_ps(_mm_cmpge_ps(V, Zero), _mm_cmple_ps(V, One))));
				WriteMask = _mm_and_si128(WriteMask, ClipMask);

				// clamp uv 
				U = _mm_min_ps(_mm_max_ps(U, Zero), One);
				V = _mm_min_ps(_mm_max_ps(V, Zero), One);
				
				__m128 tX = _mm_mul_ps(U, WidthM2);
				__m128 tY = _mm_mul_ps(V, HeightM2);

				__m128i intX = _mm_cvttps_epi32(tX);
				__m128i intY = _mm_cvttps_epi32(tY);

				__m128 fX = _mm_sub_ps(tX, _mm_cvtepi32_ps(intX));
				__m128 fY = _mm_sub_ps(tY, _mm_cvtepi32_ps(intY));
#if 1	
				__m128i FetchX_4x = _mm_slli_epi32(intX, 2);
				__m128i FetchY_4x = _mm_or_si128(_mm_mullo_epi16(intY, TexturePitch_x4),
												_mm_slli_epi32(_mm_mulhi_epi16(intY, TexturePitch_x4), 16));
				
				__m128i Fetch_4x = _mm_add_epi32(FetchX_4x, FetchY_4x);
	
				int32 Fetch0 = ((int32*)&(Fetch_4x))[0];
				int32 Fetch1 = ((int32*)&(Fetch_4x))[1];
				int32 Fetch2 = ((int32*)&(Fetch_4x))[2];
				int32 Fetch3 = ((int32*)&(Fetch_4x))[3];
		
				uint8 *TexelPtr0 = ((uint8 *) TextureMemory) + Fetch0;
				uint8 *TexelPtr1 = ((uint8 *) TextureMemory) + Fetch1;
				uint8 *TexelPtr2 = ((uint8 *) TextureMemory) + Fetch2;
				uint8 *TexelPtr3 = ((uint8 *) TextureMemory) + Fetch3;
				

				__m128i SampleA = _mm_setr_epi32(*(uint32*)TexelPtr0, *(uint32*)TexelPtr1, *(uint32*)TexelPtr2, *(uint32*)TexelPtr3);
				__m128i SampleB = _mm_setr_epi32(*(uint32*)(TexelPtr0 + sizeof(uint32)),
												*(uint32*)(TexelPtr1 + sizeof(uint32)),
												*(uint32*)(TexelPtr2 + sizeof(uint32)),
												*(uint32*)(TexelPtr3 + sizeof(uint32)));
				__m128i SampleC = _mm_setr_epi32(*(uint32*)(TexelPtr0 + Texture->Pitch),
												*(uint32*)(TexelPtr1 + Texture->Pitch),
												*(uint32*)(TexelPtr2 + Texture->Pitch),
												*(uint32*)(TexelPtr3 + Texture->Pitch));
				__m128i SampleD = _mm_setr_epi32(*(uint32*)(TexelPtr0 + Texture->Pitch + sizeof(uint32)),
												*(uint32*)(TexelPtr1 + Texture->Pitch + sizeof(uint32)),
												*(uint32*)(TexelPtr2 + Texture->Pitch + sizeof(uint32)),
												*(uint32*)(TexelPtr3 + Texture->Pitch + sizeof(uint32)));

				__m128i TexelArb = _mm_and_si128(SampleA, MaskFF00FF);
				__m128i TexelAag = _mm_and_si128(_mm_srli_epi32(SampleA, 8), MaskFF00FF);
				TexelArb = _mm_mullo_epi16(TexelArb, TexelArb);
				__m128 TexelAa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelAag, 16));
				TexelAag = _mm_mullo_epi16(TexelAag, TexelAag);

				__m128i TexelBrb = _mm_and_si128(SampleB, MaskFF00FF);
				__m128i TexelBag = _mm_and_si128(_mm_srli_epi32(SampleB, 8), MaskFF00FF);
				TexelBrb = _mm_mullo_epi16(TexelBrb, TexelBrb);
				__m128 TexelBa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBag, 16));
				TexelBag = _mm_mullo_epi16(TexelBag, TexelBag);

				__m128i TexelCrb = _mm_and_si128(SampleC, MaskFF00FF);
				__m128i TexelCag = _mm_and_si128(_mm_srli_epi32(SampleC, 8), MaskFF00FF);
				TexelCrb = _mm_mullo_epi16(TexelCrb, TexelCrb);
				__m128 TexelCa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelCag, 16));
				TexelCag = _mm_mullo_epi16(TexelCag, TexelCag);

				__m128i TexelDrb = _mm_and_si128(SampleD, MaskFF00FF);
				__m128i TexelDag = _mm_and_si128(_mm_srli_epi32(SampleD, 8), MaskFF00FF);
				TexelDrb = _mm_mullo_epi16(TexelDrb, TexelDrb);
				__m128 TexelDa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelDag, 16));
				TexelDag = _mm_mullo_epi16(TexelDag, TexelDag);
#else
				__m128i SampleA;
				__m128i SampleB;
				__m128i SampleC;
				__m128i SampleD;
				for (int I = 0; I < 4; ++I) {
					int32 FetchX = ((uint32*)&(intX))[I];
					int32 FetchY = ((uint32*)&(intY))[I];
					
					uint8* TexelPtr = ((uint8*)Texture->Memory) + Texture->Pitch * FetchY + FetchX * sizeof(uint32);
					((uint32*)&(SampleA))[I] = *(uint32*)TexelPtr;
					((uint32*)&(SampleB))[I] = *(uint32*)(TexelPtr + sizeof(uint32));
					((uint32*)&(SampleC))[I] = *(uint32*)(TexelPtr + Texture->Pitch);
					((uint32*)&(SampleD))[I] = *(uint32*)(TexelPtr + Texture->Pitch + sizeof(uint32));
				}
#endif
				__m128 TexelAr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelArb, 16));
				__m128 TexelAg = _mm_cvtepi32_ps(_mm_and_si128(TexelAag, MaskFFFF));
				__m128 TexelAb = _mm_cvtepi32_ps(_mm_and_si128(TexelArb, MaskFFFF));

				__m128 TexelBr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBrb, 16));
				__m128 TexelBg = _mm_cvtepi32_ps(_mm_and_si128(TexelBag, MaskFFFF));
				__m128 TexelBb = _mm_cvtepi32_ps(_mm_and_si128(TexelBrb, MaskFFFF));

				__m128 TexelCr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelCrb, 16));
				__m128 TexelCg = _mm_cvtepi32_ps(_mm_and_si128(TexelCag, MaskFFFF));
				__m128 TexelCb = _mm_cvtepi32_ps(_mm_and_si128(TexelCrb, MaskFFFF));

				__m128 TexelDr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelDrb, 16));
				__m128 TexelDg = _mm_cvtepi32_ps(_mm_and_si128(TexelDag, MaskFFFF));
				__m128 TexelDb = _mm_cvtepi32_ps(_mm_and_si128(TexelDrb, MaskFFFF));


				__m128 Destb = _mm_cvtepi32_ps(_mm_and_si128(OriginalDest, MaskFF));
				__m128 Destg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 8), MaskFF));
				__m128 Destr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 16), MaskFF));
				__m128 Desta = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 24), MaskFF));


	#define mmSquare(a) _mm_mul_ps(a, a)
			

				__m128 ifX = _mm_sub_ps(One, fX);
				__m128 ifY = _mm_sub_ps(One, fY);

				__m128 l1 = _mm_mul_ps(ifX, ifY);
				__m128 l2 = _mm_mul_ps(fX, ifY);
				__m128 l3 = _mm_mul_ps(ifX, fY);
				__m128 l4 = _mm_mul_ps(fX, fY);


				__m128 Texelr = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(TexelAr, l1), _mm_mul_ps(TexelBr, l2)), _mm_mul_ps(TexelCr, l3)), _mm_mul_ps(TexelDr, l4));
				__m128 Texelg = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(TexelAg, l1), _mm_mul_ps(TexelBg, l2)), _mm_mul_ps(TexelCg, l3)), _mm_mul_ps(TexelDg, l4));
				__m128 Texelb = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(TexelAb, l1), _mm_mul_ps(TexelBb, l2)), _mm_mul_ps(TexelCb, l3)), _mm_mul_ps(TexelDb, l4));
				__m128 Texela = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(TexelAa, l1), _mm_mul_ps(TexelBa, l2)), _mm_mul_ps(TexelCa, l3)), _mm_mul_ps(TexelDa, l4));

				Texelr = _mm_mul_ps(Texelr, ColorR);
				Texelg = _mm_mul_ps(Texelg, ColorG);
				Texelb = _mm_mul_ps(Texelb, ColorB);
				Texela = _mm_mul_ps(Texela, ColorA);

				Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero), MaxColorValue);
				Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero), MaxColorValue);
				Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero), MaxColorValue);

				

				Destr = mmSquare(Destr);
				Destg = mmSquare(Destg);
				Destb = mmSquare(Destb);

				__m128 InvTexelA = _mm_sub_ps(One, _mm_mul_ps(Inv255C_x4, Texela));
				__m128 Blendedr = _mm_add_ps(_mm_mul_ps(InvTexelA, Destr), Texelr);
				__m128 Blendedg = _mm_add_ps(_mm_mul_ps(InvTexelA, Destg), Texelg);
				__m128 Blendedb = _mm_add_ps(_mm_mul_ps(InvTexelA, Destb), Texelb);
				__m128 Blendeda = _mm_add_ps(_mm_mul_ps(InvTexelA, Desta), Texela);

				Blendedr = _mm_mul_ps(Blendedr, _mm_rsqrt_ps(Blendedr));
				Blendedg = _mm_mul_ps(Blendedg, _mm_rsqrt_ps(Blendedg));
				Blendedb = _mm_mul_ps(Blendedb, _mm_rsqrt_ps(Blendedb));

				__m128i IntR = _mm_cvtps_epi32(Blendedr);
				__m128i IntG = _mm_cvtps_epi32(Blendedg);
				__m128i IntB = _mm_cvtps_epi32(Blendedb);
				__m128i IntA = _mm_cvtps_epi32(Blendeda);

				__m128i Sr = _mm_slli_epi32(IntR, 16);
				__m128i Sg = _mm_slli_epi32(IntG, 8);
				__m128i Sb = IntB;
				__m128i Sa = _mm_slli_epi32(IntR, 24);

				__m128i Out = _mm_or_si128(_mm_or_si128(Sr, Sg), _mm_or_si128(Sb, Sa));
	#if 1
				Out = _mm_or_si128(_mm_and_si128(WriteMask, Out), _mm_andnot_si128(WriteMask, OriginalDest));
	#else
				Out = _mm_and_si128(WriteMask, Out);
	#endif
				_mm_store_si128((__m128i*)Pixel, Out);

				tempPx = _mm_add_ps(tempPx, Four_x4);
				Pixel += 4;
				if ((X + 8) < FillRect.MaxX) {
					ClipMask = _mm_set1_epi8(-1);
				}
				else {
					ClipMask = EndClipMask;
				}
			}
			Row += RowAdvance;
		}
		END_TIMED_BLOCK(FillPixel);
	}
}

internal void
DrawRectangle(loaded_bitmap* Buffer,
	v2 vMin, v2 vMax,
	v4 Color, rectangle2i ClipRect, bool32 Even) {

	real32 R = Color.r;
	real32 G = Color.g;
	real32 B = Color.b;
	real32 A = Color.a;
	
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
	uint32 Color32 = (uint32)
		(RoundReal32ToUInt32(A * 255) << 24 |
			RoundReal32ToUInt32(R * 255) << 16 |
			RoundReal32ToUInt32(G * 255) << 8 |
			RoundReal32ToUInt32(B * 255) << 0);

	uint8* Row = ((uint8*)Buffer->Memory + FillRect.MinX * BITMAP_BYTE_PER_PIXEL + FillRect.MinY * Buffer->Pitch);

	for (int Y = FillRect.MinY; Y < FillRect.MaxY; Y += 2) {
		uint32* Pixel = (uint32*)Row;
		for (int X = FillRect.MinX; X < FillRect.MaxX; ++X) {
			*Pixel++ = Color32;
		}
		Row += 2 * Buffer->Pitch;
	}
}


internal void
RenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget, rectangle2i ClipRect, bool32 Even) {
	BEGIN_TIMED_BLOCK(RenderGroupToOutput);
	v2 ScreenDim = { (real32)OutputTarget->Width, (real32)OutputTarget->Height };
	for (uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize;) {
		render_group_entry_header* Header = (render_group_entry_header*)(RenderGroup->PushBufferBase + BaseAddress);
		BaseAddress += sizeof(*Header);
		void* Data = Header + 1;

		//todo null
		real32 NullPixelsToMeters = 1.0f;

		switch (Header->Type) {
		case RenderGroupEntryType_render_entry_clear: {
			render_entry_clear* Entry = (render_entry_clear*)Data;

			DrawRectangle(OutputTarget, v2{ 0, 0 }, v2{ (real32)OutputTarget->Width, (real32)OutputTarget->Height }, Entry->Color, ClipRect, Even);
			BaseAddress += sizeof(*Entry);
		} break;

		case RenderGroupEntryType_render_entry_rectangle: {
			render_entry_rectangle* Entry = (render_entry_rectangle*)Data;
			
			DrawRectangle(OutputTarget, Entry->P, Entry->P + Entry->Dim, Entry->Color, ClipRect, Even);

			BaseAddress += sizeof(*Entry);
		} break;
		case RenderGroupEntryType_render_entry_bitmap: {
			render_entry_bitmap* Entry = (render_entry_bitmap*)Data;
			DrawRectangle2(OutputTarget, Entry->P, V2(Entry->Size.x, 0), V2(0, Entry->Size.y), Entry->Color, Entry->Bitmap, NullPixelsToMeters, ClipRect, Even);
				
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
	END_TIMED_BLOCK(RenderGroupToOutput);
}

struct tile_render_work {
	render_group* RenderGroup;
	loaded_bitmap* OutputTarget;
	rectangle2i ClipRect;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(TileRenderToOutput) {
	tile_render_work* Work = (tile_render_work*)Data;

	RenderGroupToOutput(Work->RenderGroup, Work->OutputTarget, Work->ClipRect, false);
	RenderGroupToOutput(Work->RenderGroup, Work->OutputTarget, Work->ClipRect, true);
}


internal void 
TiledRenderGroupToOutput(platform_work_queue* Queue, render_group* RenderGroup, loaded_bitmap* OutputTarget) {
	uint32 const TileCountX = 4;
	uint32 const TileCountY = 4;


	Assert(((uintptr)OutputTarget->Memory & 15) == 0);
	uint32 TileWidth = OutputTarget->Width / TileCountX;
	uint32 TileHeight = OutputTarget->Height / TileCountY;
	TileWidth = ((TileWidth + 3) / 4) * 4;
	#if 1
	tile_render_work Works[TileCountX * TileCountY];
	for (uint32 Y = 0; Y < TileCountY; Y++) {
		for (uint32 X = 0; X < TileCountX; X++) {

			
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

			PlatformAddEntry(Queue, TileRenderToOutput, Work);
		}
	}
	PlatformCompleteAllWork(Queue);
	#else
	rectangle2i Cilp = { 0, 0, OutputTarget->Width, OutputTarget->Height };
	RenderGroupToOutput(RenderGroup, OutputTarget, Cilp, false);
		
		
	#endif
	
	
}
