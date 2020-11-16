
#include "handmade.h"

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
PushBitmap(render_group* Group, loaded_bitmap* Bitmap, real32 Height,
	v3 Offset, v4 Color = v4{ 1.0f, 1.0, 1.0f, 1.0f }) {
	render_entry_bitmap* Entry = PushRenderElement(Group, render_entry_bitmap);
	if (Entry) {
		Entry->EntityBasis.Basis = Group->DefaultBasis;
		Entry->Bitmap = Bitmap;
		v2 Size = v2{ Height * Bitmap->WidthOverHeight, Height };
		v2 Align = Hadamard(Size, Bitmap->AlignPercentage);
		Entry->EntityBasis.Offset = Offset - V3(Align, 0.0f);
		Entry->Color = Group->GlobalAlpha * Color;
		Entry->Size = Size;
	}
}

inline void
PushRect(render_group* Group, v3 Offset, v2 Dim, v4 Color) {
	render_entry_rectangle* Entry = PushRenderElement(Group, render_entry_rectangle);
	if (Entry) {
		Entry->EntityBasis.Basis = Group->DefaultBasis;
		Entry->EntityBasis.Offset = Offset - V3(0.5f * Dim, 0);
		Entry->Color = Color;
		Entry->Dim = Dim;
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

	Result->PushBufferBase = (uint8*)PushSize(Arena, MaxPushBufferSize);
	Result->MaxPushBufferSize = MaxPushBufferSize;

	Result->DefaultBasis = PushStruct(Arena, render_basis);
	Result->DefaultBasis->P = v3{ 0, 0, 0 };

	Result->PushBufferSize = 0;
	Result->GlobalAlpha = 1.0f;

	Result->GameCamera.FocalLength = 0.6f;
	Result->GameCamera.DistanceAboveTarget = 9.0f;

	Result->RenderCamera = Result->GameCamera;
	//Result->RenderCamera.DistanceAboveTarget = 50.0f;

	real32 WidthOfMonitor = 0.635f;
	Result->MetersToPixels = (real32)ResolutionPixelX * WidthOfMonitor;
	real32 PixelsToMeters = 1.0f / Result->MetersToPixels;
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
DrawRectangle2(loaded_bitmap* Buffer, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture, real32 PixelsToMeters) {
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
	int32 WidthMax = Buffer->Width - 1 - 3;
	int32 HeightMax = Buffer->Height - 1 - 3;

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
	real32 Inv255C = 1.0f / 255.0f;
	real32 One255C = 255.0f;
	__m128 Inv255C_x4 = _mm_set_ps1(Inv255C);
	__m128 One255C_x4 = _mm_set_ps1(One255C);
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

	__m128 InvAxisXLengthSq_x4 = _mm_set_ps1(InvAxisXLengthSq);
	__m128 InvAxisYLengthSq_x4 = _mm_set_ps1(InvAxisYLengthSq);
	__m128 NAxisXx = _mm_mul_ps(XAxisx, InvAxisXLengthSq_x4);
	__m128 NAxisXy = _mm_mul_ps(XAxisy, InvAxisXLengthSq_x4);
	__m128 NAxisYx = _mm_mul_ps(YAxisx, InvAxisYLengthSq_x4);
	__m128 NAxisYy = _mm_mul_ps(YAxisy, InvAxisYLengthSq_x4);
	BEGIN_TIMED_BLOCK(FillPixel);
	for (int Y = MinY; Y < MaxY; ++Y) {
		uint32* Pixel = (uint32*)Row;
		for (int X = MinX; X < MaxX; X += 4) {
			// load

			__m128 X_x4 = _mm_set_ps1((real32)X);
			__m128 Y_x4 = _mm_set_ps1((real32)Y);
			__m128 Destr = _mm_set_ps1(0.0f);
			__m128 Destg = _mm_set_ps1(0.0f);
			__m128 Destb = _mm_set_ps1(0.0f);
			__m128 Desta = _mm_set_ps1(0.0f);

			__m128 TexelAr = _mm_set_ps1(0.0f);
			__m128 TexelAg = _mm_set_ps1(0.0f);
			__m128 TexelAb = _mm_set_ps1(0.0f);
			__m128 TexelAa = _mm_set_ps1(0.0f);

			__m128 TexelBr = _mm_set_ps1(0.0f);
			__m128 TexelBg = _mm_set_ps1(0.0f);
			__m128 TexelBb = _mm_set_ps1(0.0f);
			__m128 TexelBa = _mm_set_ps1(0.0f);

			__m128 TexelCr = _mm_set_ps1(0.0f);
			__m128 TexelCg = _mm_set_ps1(0.0f);
			__m128 TexelCb = _mm_set_ps1(0.0f);
			__m128 TexelCa = _mm_set_ps1(0.0f);

			__m128 TexelDr = _mm_set_ps1(0.0f);
			__m128 TexelDg = _mm_set_ps1(0.0f);
			__m128 TexelDb = _mm_set_ps1(0.0f);
			__m128 TexelDa = _mm_set_ps1(0.0f);

			__m128 Blendedr = _mm_set_ps1(0.0f);
			__m128 Blendedg = _mm_set_ps1(0.0f);
			__m128 Blendedb = _mm_set_ps1(0.0f);
			__m128 Blendeda = _mm_set_ps1(0.0f);

			__m128 fX = _mm_set_ps1(0.0f);
			__m128 fY = _mm_set_ps1(0.0f);

			__m128 tempPx = _mm_set_ps((real32)(X + 3), (real32)(X + 2), (real32)(X + 1), (real32)(X + 0));
			__m128 Dx = _mm_sub_ps(tempPx, OriginX_x4);
			__m128 Dy = _mm_sub_ps(Y_x4, OriginY_x4);

			__m128 U = _mm_add_ps(_mm_mul_ps(Dx, NAxisXx), _mm_mul_ps(Dy, NAxisXy));
			__m128 V = _mm_add_ps(_mm_mul_ps(Dx, NAxisYx), _mm_mul_ps(Dy, NAxisYy));
			bool ShouldTest[4];
			for (int I = 0; I < 4; ++I) {
				ShouldTest[I] = (((float*)&(U))[I] >= 0.0f && ((float*)&(U))[I] <= 1.0f && ((float*)&(V))[I] >= 0.0f && ((float*)&(V))[I] <= 1.0f);
				if (ShouldTest[I]) {
					real32 tX = ((float*)&(U))[I] * (real32)(Texture->Width - 2);
					real32 tY = ((float*)&(V))[I] * (real32)(Texture->Height - 2);

					int32 intX = (int32)tX;
					int32 intY = (int32)tY;

					((float*)&(fX))[I] = tX - (real32)intX;
					((float*)&(fY))[I] = tY - (real32)intY;

					uint8* TexelPtr = (uint8*)Texture->Memory + Texture->Pitch * intY + intX * sizeof(uint32);
					uint32 SampleA = *(uint32*)TexelPtr;
					uint32 SampleB = *(uint32*)(TexelPtr + sizeof(uint32));
					uint32 SampleC = *(uint32*)(TexelPtr + Texture->Pitch);
					uint32 SampleD = *(uint32*)(TexelPtr + Texture->Pitch + sizeof(uint32));

					((float*)&(TexelAr))[I] = (real32)((SampleA >> 16) & 0xFF);
					((float*)&(TexelAg))[I] = (real32)((SampleA >> 8) & 0xFF);
					((float*)&(TexelAb))[I] = (real32)((SampleA >> 0) & 0xFF);
					((float*)&(TexelAa))[I] = (real32)((SampleA >> 24) & 0xFF);

					((float*)&(TexelBr))[I] = (real32)((SampleB >> 16) & 0xFF);
					((float*)&(TexelBg))[I] = (real32)((SampleB >> 8) & 0xFF);
					((float*)&(TexelBb))[I] = (real32)((SampleB >> 0) & 0xFF);
					((float*)&(TexelBa))[I] = (real32)((SampleB >> 24) & 0xFF);

					((float*)&(TexelCr))[I] = (real32)((SampleC >> 16) & 0xFF);
					((float*)&(TexelCg))[I] = (real32)((SampleC >> 8) & 0xFF);
					((float*)&(TexelCb))[I] = (real32)((SampleC >> 0) & 0xFF);
					((float*)&(TexelCa))[I] = (real32)((SampleC >> 24) & 0xFF);

					((float*)&(TexelDr))[I] = (real32)((SampleD >> 16) & 0xFF);
					((float*)&(TexelDg))[I] = (real32)((SampleD >> 8) & 0xFF);
					((float*)&(TexelDb))[I] = (real32)((SampleD >> 0) & 0xFF);
					((float*)&(TexelDa))[I] = (real32)((SampleD >> 24) & 0xFF);

					((float*)&(Destr))[I] = (real32)((*(Pixel + I) >> 16) & 0xFF);
					((float*)&(Destg))[I] = (real32)((*(Pixel + I) >> 8) & 0xFF);
					((float*)&(Destb))[I] = (real32)((*(Pixel + I) >> 0) & 0xFF);
					((float*)&(Desta))[I] = (real32)((*(Pixel + I) >> 24) & 0xFF);
					//////////
				}
			}
#define mmSquare(a) _mm_mul_ps(a, a)
			TexelAr = mmSquare(_mm_mul_ps(Inv255C_x4, TexelAr));
			TexelAg = mmSquare(_mm_mul_ps(Inv255C_x4, TexelAg));
			TexelAb = mmSquare(_mm_mul_ps(Inv255C_x4, TexelAb));
			TexelAa = _mm_mul_ps(Inv255C_x4, TexelAa);

			TexelBr = mmSquare(_mm_mul_ps(Inv255C_x4, TexelBr));
			TexelBg = mmSquare(_mm_mul_ps(Inv255C_x4, TexelBg));
			TexelBb = mmSquare(_mm_mul_ps(Inv255C_x4, TexelBb));
			TexelBa = _mm_mul_ps(Inv255C_x4, TexelBa);

			TexelCr = mmSquare(_mm_mul_ps(Inv255C_x4, TexelCr));
			TexelCg = mmSquare(_mm_mul_ps(Inv255C_x4, TexelCg));
			TexelCb = mmSquare(_mm_mul_ps(Inv255C_x4, TexelCb));
			TexelCa = _mm_mul_ps(Inv255C_x4, TexelCa);

			TexelDr = mmSquare(_mm_mul_ps(Inv255C_x4, TexelDr));
			TexelDg = mmSquare(_mm_mul_ps(Inv255C_x4, TexelDg));
			TexelDb = mmSquare(_mm_mul_ps(Inv255C_x4, TexelDb));
			TexelDa = _mm_mul_ps(Inv255C_x4, TexelDa);

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

			Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero), One);
			Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero), One);
			Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero), One);

			Destr = _mm_mul_ps(Destr, Inv255C_x4);
			Destr = _mm_mul_ps(Destr, Destr);
			Destg = _mm_mul_ps(Destg, Inv255C_x4);
			Destg = _mm_mul_ps(Destg, Destg);
			Destb = _mm_mul_ps(Destb, Inv255C_x4);
			Destb = _mm_mul_ps(Destb, Destb);
			Desta = _mm_mul_ps(Desta, Inv255C_x4);

			__m128 InvTexelA = _mm_sub_ps(One, Texela);

			Blendedr = _mm_add_ps(_mm_mul_ps(InvTexelA, Destr), Texelr);
			Blendedg = _mm_add_ps(_mm_mul_ps(InvTexelA, Destg), Texelg);
			Blendedb = _mm_add_ps(_mm_mul_ps(InvTexelA, Destb), Texelb);
			Blendeda = _mm_add_ps(_mm_mul_ps(InvTexelA, Desta), Texela);

			Blendedr = _mm_mul_ps(One255C_x4, _mm_sqrt_ps(Blendedr));
			Blendedg = _mm_mul_ps(One255C_x4, _mm_sqrt_ps(Blendedg));
			Blendedb = _mm_mul_ps(One255C_x4, _mm_sqrt_ps(Blendedb));
			Blendeda = _mm_mul_ps(One255C_x4, Blendeda);


			for (int I = 0; I < 4; ++I) {
				if (ShouldTest[I]) {
					*(Pixel + I) = (uint32)(((float*)&(Blendeda))[I] + 0.5f) << 24 |
						(uint32)(((float*)&(Blendedr))[I] + 0.5f) << 16 |
						(uint32)(((float*)&(Blendedg))[I] + 0.5f) << 8 |
						(uint32)(((float*)&(Blendedb))[I] + 0.5f) << 0;
				}
			}

			Pixel += 4;
		}
		Row += Buffer->Pitch;
	}
	END_TIMED_BLOCK_COUNTED(FillPixel, (MaxX-MinX+1)*(MaxY-MinY+1));
}

internal void
DrawRectangle(loaded_bitmap* Buffer,
	v2 vMin, v2 vMax,
	v4 Color) {

	real32 R = Color.r;
	real32 G = Color.g;
	real32 B = Color.b;
	real32 A = Color.a;

	int32 MinX = RoundReal32ToInt32(vMin.x);
	int32 MinY = RoundReal32ToInt32(vMin.y);
	int32 MaxX = RoundReal32ToInt32(vMax.x);
	int32 MaxY = RoundReal32ToInt32(vMax.y);

	if (MinX < 0) {
		MinX = 0;
	}

	if (MinY < 0) {
		MinY = 0;
	}
	if (MaxY > Buffer->Height) {
		MaxY = Buffer->Height;
	}
	if (MaxX > Buffer->Width) {
		MaxX = Buffer->Width;
	}

	// BIT PATTERN AA RR GG BB
	uint32 Color32 = (uint32)
		(RoundReal32ToUInt32(A * 255) << 24 |
			RoundReal32ToUInt32(R * 255) << 16 |
			RoundReal32ToUInt32(G * 255) << 8 |
			RoundReal32ToUInt32(B * 255) << 0);

	uint8* Row = ((uint8*)Buffer->Memory + MinX * BITMAP_BYTE_PER_PIXEL + MinY * Buffer->Pitch);

	for (int Y = MinY; Y < MaxY; ++Y) {
		uint32* Pixel = (uint32*)Row;
		for (int X = MinX; X < MaxX; ++X) {
			*Pixel++ = Color32;
		}
		Row += Buffer->Pitch;
	}
}


struct entity_basis_p_result {
	v2 P;
	real32 Scale;
};

internal entity_basis_p_result
GetRenderEntityBasisP(render_group* RenderGroup, render_entity_basis* EntityBasis, v2 ScreenDim) {
	v2 ScreenCenter = 0.5f * ScreenDim;
	entity_basis_p_result Result = {};
	v3 EntityBaseP = EntityBasis->Basis->P;

	real32 DistancePz = RenderGroup->RenderCamera.DistanceAboveTarget - EntityBaseP.z;
	v3 RawXY = V3(EntityBaseP.xy + EntityBasis->Offset.xy, 1.0f);
	real32 NearClipPlane = 0.2f;

	if (DistancePz > NearClipPlane) {
		v3 ProjectedXY = 1.0f / DistancePz * RenderGroup->RenderCamera.FocalLength * RawXY;
		v2 EntityGround = ScreenCenter + RenderGroup->MetersToPixels * ProjectedXY.xy;
		Result.P = EntityGround;
		Result.Scale = RenderGroup->MetersToPixels * ProjectedXY.z;
	}

	return(Result);
}

internal void
RenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget) {
	BEGIN_TIMED_BLOCK(RenderGroupToOutput);
	v2 ScreenDim = { (real32)OutputTarget->Width, (real32)OutputTarget->Height };
	for (uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize;) {
		render_group_entry_header* Header = (render_group_entry_header*)(RenderGroup->PushBufferBase + BaseAddress);
		BaseAddress += sizeof(*Header);
		void* Data = Header + 1;
		real32 PixelsToMeters = 1.0f / RenderGroup->MetersToPixels;
		switch (Header->Type) {
		case RenderGroupEntryType_render_entry_clear: {
			render_entry_clear* Entry = (render_entry_clear*)Data;

			DrawRectangle(OutputTarget, v2{ 0, 0 }, v2{ (real32)OutputTarget->Width, (real32)OutputTarget->Height }, Entry->Color);
			BaseAddress += sizeof(*Entry);
		} break;

		case RenderGroupEntryType_render_entry_rectangle: {
			render_entry_rectangle* Entry = (render_entry_rectangle*)Data;
			entity_basis_p_result Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenDim);
			DrawRectangle(OutputTarget, Basis.P, Basis.P + Basis.Scale * Entry->Dim, Entry->Color);

			BaseAddress += sizeof(*Entry);
		} break;
		case RenderGroupEntryType_render_entry_bitmap: {
			render_entry_bitmap* Entry = (render_entry_bitmap*)Data;
			entity_basis_p_result Basis = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenDim);
			DrawRectangle2(OutputTarget, Basis.P,
				Basis.Scale * V2(Entry->Size.x, 0),
				Basis.Scale * V2(0, Entry->Size.y), Entry->Color,
				Entry->Bitmap, PixelsToMeters);
			BaseAddress += sizeof(*Entry);
		} break;
		case RenderGroupEntryType_render_entry_coordinate_system: {
			render_entry_coordinate_system* Entry = (render_entry_coordinate_system*)Data;
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

			BaseAddress += sizeof(*Entry);
		} break;
			InvalidDefaultCase;
		}
	}
	END_TIMED_BLOCK(RenderGroupToOutput);
}