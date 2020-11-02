
#include "handmade_render_group.h"
#include "handmade.h"

#define PushRenderElement(Group, type)(type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type)


inline render_group_entry_header*
PushRenderElement_(render_group* Group, uint32 Size, render_group_entry_type Type) {
	render_group_entry_header* Result = 0;
	if (Group->PushBufferSize + Size < Group->MaxPushBufferSize) {
		Result = (render_group_entry_header*)(Group->PushBufferBase + Group->PushBufferSize);
		Result->Type = Type;
		Group->PushBufferSize += Size;
	}
	else {
		InvalidCodePath;
	}
	return(Result);
}

inline void
PushPiece(render_group* Group, loaded_bitmap* Bitmap,
	v2 Offset, real32 OffsetZ, v2 Align, v2 Dim, v4 Color, real32 EntityZC) {
	render_entry_bitmap* Piece = PushRenderElement(Group, render_entry_bitmap);
	if (Piece) {
		Piece->EntityBasis.Basis = Group->DefaultBasis;
		Piece->Bitmap = Bitmap;

		Piece->EntityBasis.Offset = Group->MetersToPixels * v2{ Offset.x, -Offset.y } -Align;
		Piece->EntityBasis.OffsetZ = OffsetZ;
		Piece->EntityBasis.EntityZC = EntityZC;

		Piece->R = Color.r;
		Piece->G = Color.g;
		Piece->B = Color.b;
		Piece->A = Color.a;
	}
}

inline void
PushBitmap(render_group* Group, loaded_bitmap* Bitmap,
	v2 Offset, real32 OffsetZ, v2 Align, real32 Alpha = 1.0f, real32 EntityZC = 1.0f) {
	PushPiece(Group, Bitmap, Offset, OffsetZ, Align,
		v2{ 0, 0 }, v4{ 1.0f, 1.0f, 1.0f, Alpha }, EntityZC);
}

inline void
PushRect(render_group* Group, v2 Offset, real32 OffsetZ,
	v2 Dim, v4 Color, real32 EntityZC = 1.0f) {
	render_entry_rectangle* Piece = PushRenderElement(Group, render_entry_rectangle);
	if (Piece) {
		v2 HalfDim = 0.5f * Group->MetersToPixels * Dim;

		Piece->EntityBasis.Basis = Group->DefaultBasis;
		Piece->EntityBasis.Offset = Group->MetersToPixels * v2{ Offset.x, -Offset.y } - HalfDim;
		Piece->EntityBasis.OffsetZ = OffsetZ;
		Piece->EntityBasis.EntityZC = EntityZC;

		Piece->R = Color.r;
		Piece->G = Color.g;
		Piece->B = Color.b;
		Piece->A = Color.a;

		Piece->Dim = Group->MetersToPixels * Dim;
	}
}

inline void
PushRectOutline(render_group* Group, v2 Offset, real32 OffsetZ,
	v2 Dim, v4 Color, real32 EntityZC = 1.0f) {

	real32 Thickness = 0.1f;
	// top - bottom
	PushRect(Group, Offset - v2{ 0, 0.5f * Dim.y }, OffsetZ, v2{ Dim.x, Thickness }, Color, EntityZC);
	PushRect(Group, Offset + v2{ 0, 0.5f * Dim.y }, OffsetZ, v2{ Dim.x, Thickness }, Color, EntityZC);

	PushRect(Group, Offset - v2{ 0.5f * Dim.x, 0 }, OffsetZ, v2{ Thickness, Dim.y }, Color, EntityZC);
	PushRect(Group, Offset + v2{ 0.5f * Dim.x, 0 }, OffsetZ, v2{ Thickness, Dim.y }, Color, EntityZC);
}

inline void
Clear(render_group* Group, v4 Color) {
	render_entry_clear* Piece = PushRenderElement(Group, render_entry_clear);
	if (Piece) {
		Piece->Color = Color;
	}
}

render_entry_coordinate_system*
CoordinateSystem(render_group* Group, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap *Texture) {
	render_entry_coordinate_system* Entry = PushRenderElement(Group, render_entry_coordinate_system);
	if (Entry) {
		Entry->AxisX = AxisX;
		Entry->AxisY = AxisY;
		Entry->Color = Color;
		Entry->Origin = Origin;
		Entry->Texture = Texture;
	}
	return(Entry);
}

render_group*
AllocateRenderGroup(memory_arena* Arena, uint32 MaxPushBufferSize, real32 MetersToPixels) {
	render_group* Result = PushStruct(Arena, render_group);

	Result->MetersToPixels = MetersToPixels;

	Result->PushBufferBase = (uint8*)PushSize(Arena, MaxPushBufferSize);
	Result->MaxPushBufferSize = MaxPushBufferSize;

	Result->DefaultBasis = PushStruct(Arena, render_basis);
	Result->DefaultBasis->P = v3{ 0, 0, 0 };

	Result->PushBufferSize = 0;
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
			++Dest;
			++Source;
		}
		DestRow += Buffer->Pitch;
		SourceRow += Bitmap->Pitch;
	}
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
DrawRectangle1(loaded_bitmap* Buffer, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture) {
#if 0
	uint32 Color32 = (uint32)(RoundReal32ToUInt32(Color.r * 255) << 16 |
		RoundReal32ToUInt32(Color.g * 255) << 8 |
		RoundReal32ToUInt32(Color.b * 255) << 0);
#endif

	real32 InvAxisXLengthSq = 1.0f / LengthSq(AxisX);
	real32 InvAxisYLengthSq = 1.0f / LengthSq(AxisY);
	int32 WidthMax = Buffer->Width - 1;
	int32 HeightMax = Buffer->Height - 1;
	int32 MinX = WidthMax;
	int32 MaxX = 0;
	int32 MinY = HeightMax;
	int32 MaxY = 0;
	
	v2 P[4] = { Origin, Origin + AxisX, Origin + AxisX + AxisY, Origin + AxisY };
	
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

			if (Edge0 < 0 &&
				Edge1 < 0 &&
				Edge2 < 0 &&
				Edge3 < 0
				) {
				real32 RealU = (InvAxisXLengthSq * Inner(D, AxisX) * (real32)(Texture->Width - 2));
				real32 RealV = (InvAxisYLengthSq * Inner(D, AxisY) * (real32)(Texture->Height - 2));

				int32 U = (int32)RealU;
				int32 V = (int32)RealV;

				real32 dU = RealU - (real32)U;
				real32 dV = RealV - (real32)V;

				uint8* Color8 = (uint8*)Texture->Memory + Texture->Pitch * V + U * sizeof(uint32);
				uint32 ColorA = *(uint32*)Color8;
				uint32 ColorB = *(uint32*)(Color8 + sizeof(uint32));

				uint32 ColorC = *(uint32*)(Color8 + Texture->Pitch);
				uint32 ColorD = *(uint32*)(Color8 + Texture->Pitch + sizeof(uint32));

				v4 TexelA = v4{
					(real32)((ColorA >> 16) & 0xFF),
					(real32)((ColorA >> 8) & 0xFF),
					(real32)((ColorA >> 0) & 0xFF),
					(real32)((ColorA >> 24) & 0xFF)
				};
				v4 TexelB = v4{
					(real32)((ColorB >> 16) & 0xFF),
					(real32)((ColorB >> 8) & 0xFF),
					(real32)((ColorB >> 0) & 0xFF),
					(real32)((ColorB >> 24) & 0xFF)
				};
				v4 TexelC = v4{
					(real32)((ColorC >> 16) & 0xFF),
					(real32)((ColorC >> 8) & 0xFF),
					(real32)((ColorC >> 0) & 0xFF),
					(real32)((ColorC >> 24) & 0xFF)
				};
				v4 TexelD = v4{
					(real32)((ColorD >> 16) & 0xFF),
					(real32)((ColorD >> 8) & 0xFF),
					(real32)((ColorD >> 0) & 0xFF),
					(real32)((ColorD >> 24) & 0xFF)
				};
				TexelA = SRGBToLinear1(TexelA);
				TexelB = SRGBToLinear1(TexelB);
				TexelC = SRGBToLinear1(TexelC);
				TexelD = SRGBToLinear1(TexelD);

				v4 Texel = Lerp(Lerp(TexelA, dU, TexelB), dV, Lerp(TexelC, dU, TexelD));

				real32 RSA = Texel.a * Color.a;

				v4 Dest = {
					(real32)((*Pixel >> 16) & 0xFF),
					(real32)((*Pixel >> 8) & 0xFF),
					(real32)((*Pixel >> 0) & 0xFF),
					(real32)((*Pixel >> 24) & 0xFF)
				};

				Dest = SRGBToLinear1(Dest);
				real32 RDA = Dest.a;
				real32 InvRSA = (1.0f - RSA);

				
				v4 Blended = {
					InvRSA * Dest.r + Color.a * Color.r * Texel.r,
					InvRSA * Dest.g + Color.a * Color.g * Texel.g,
					InvRSA * Dest.b + Color.a * Color.b * Texel.b,
					RSA + RDA - RSA * RDA
				};

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
DrawRectangle(loaded_bitmap* Buffer,
	v2 vMin, v2 vMax,
	real32 R, real32 G, real32 B) {
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
	uint32 Color = (uint32)(RoundReal32ToUInt32(R * 255) << 16 | 
		RoundReal32ToUInt32(G * 255) << 8 |
		RoundReal32ToUInt32(B * 255) << 0);

	uint8* Row = ((uint8*)Buffer->Memory + MinX * BITMAP_BYTE_PER_PIXEL + MinY * Buffer->Pitch);

	for (int Y = MinY; Y < MaxY; ++Y) {
		uint32* Pixel = (uint32*)Row;
		for (int X = MinX; X < MaxX; ++X) {
			*Pixel++ = Color;
		}
		Row += Buffer->Pitch;
	}
}



internal v2
GetRenderEntityBasisP(render_group* RenderGroup, render_entity_basis* EntityBasis, v2 ScreenCenter) {
	v3 EntityBaseP = EntityBasis->Basis->P;
	real32 ZFudge = (1.0f + 0.1f * (EntityBaseP.z + EntityBasis->OffsetZ));
	real32 EntityGroundX = ScreenCenter.x + RenderGroup->MetersToPixels * ZFudge * EntityBaseP.x;
	real32 EntityGroundY = ScreenCenter.y - RenderGroup->MetersToPixels * ZFudge * EntityBaseP.y;
	real32 EntityZ = -EntityBaseP.z * RenderGroup->MetersToPixels;
	v2 Center = {
		EntityGroundX + EntityBasis->Offset.x,
		EntityGroundY + EntityBasis->Offset.y + EntityZ * EntityBasis->EntityZC
	};
	return(Center);
}

internal void
RenderGroupToOutput(render_group* RenderGroup, loaded_bitmap* OutputTarget) {
	v2 ScreenCenter = {
		0.5f * (real32)OutputTarget->Width,
		0.5f * (real32)OutputTarget->Height
	};
	for (uint32 BaseAddress = 0; BaseAddress < RenderGroup->PushBufferSize;) {
		render_group_entry_header* Header = (render_group_entry_header*)(RenderGroup->PushBufferBase + BaseAddress);
		switch (Header->Type) {
			case RenderGroupEntryType_render_entry_clear: {
				render_entry_clear* Entry = (render_entry_clear*)Header;

				DrawRectangle(OutputTarget, v2{ 0, 0 }, v2{ (real32)OutputTarget->Width, (real32)OutputTarget->Height }, Entry->Color.r, Entry->Color.g, Entry->Color.b);
				BaseAddress += sizeof(*Entry);
			} break;

			case RenderGroupEntryType_render_entry_rectangle: {
				render_entry_rectangle* Entry = (render_entry_rectangle*)Header;
				v2 P = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenCenter);
				DrawRectangle(OutputTarget, P, P + Entry->Dim, Entry->R, Entry->G, Entry->B);

				BaseAddress += sizeof(*Entry);
			} break;
			case RenderGroupEntryType_render_entry_bitmap: {
				render_entry_bitmap* Entry = (render_entry_bitmap*)Header;
				v2 P = GetRenderEntityBasisP(RenderGroup, &Entry->EntityBasis, ScreenCenter);
				DrawBitmap(OutputTarget, Entry->Bitmap, P.x, P.y, Entry->A);
				BaseAddress += sizeof(*Entry);
			} break;
			case RenderGroupEntryType_render_entry_coordinate_system: {
				render_entry_coordinate_system* Entry = (render_entry_coordinate_system*)Header;
				v2 P = Entry->Origin;
				
				DrawRectangle1(OutputTarget, P, Entry->AxisX, Entry->AxisY, Entry->Color, Entry->Texture);
				v2 Dim = { 2, 2 };
				v4 Color = { 1.0f, 1.0f, 0.0f, 1.0f };
				DrawRectangle(OutputTarget, P - Dim, P + Dim, Color.r, Color.g, Color.b);

				P = Entry->Origin + Entry->AxisX;
				DrawRectangle(OutputTarget, P - Dim, P + Dim, Color.r, Color.g, Color.b);

				P = Entry->Origin + Entry->AxisY;
				DrawRectangle(OutputTarget, P - Dim, P + Dim, Color.r, Color.g, Color.b);
				P = Entry->Origin + Entry->AxisY + Entry->AxisX;

				DrawRectangle(OutputTarget, P - Dim, P + Dim, Color.r, Color.g, Color.b);

				BaseAddress += sizeof(*Entry);
			} break;
			InvalidDefaultCase;
		}
	}
}