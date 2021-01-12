#define internal
#include "handmade.h"

void
DrawRectangle2(loaded_bitmap* Buffer, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture, r32 PixelsToMeters, rectangle2i ClipRect, b32 Even) {
    TIMED_FUNCTION();
    Assert(Texture->Memory);
	// premultiply alpha
	Color.rgb *= Color.a;
    
	r32 AxisXLength = Length(AxisX);
	r32 AxisYLength = Length(AxisY);
    
	v2 NxAxis = (AxisYLength / AxisXLength) * AxisX;
	v2 NyAxis = (AxisXLength / AxisYLength) * AxisY;
    
	r32 NzScale = 0.5f * (AxisXLength + AxisYLength);
    
	r32 InvAxisXLengthSq = 1.0f / LengthSq(AxisX);
	r32 InvAxisYLengthSq = 1.0f / LengthSq(AxisY);
    
	rectangle2i FillRect = InvertedInfinityRectangle2i();
    
	v2 P[4] = { Origin, Origin + AxisX, Origin + AxisX + AxisY, Origin + AxisY };
    
	for (s32 PIndex = 0; PIndex < ArrayCount(P); ++PIndex) {
		v2 TestP = P[PIndex];
		s32 FloorX = FloorReal32ToInt32(TestP.x);
		s32 CeilX = CeilReal32ToInt32(TestP.x) + 1;
        
		s32 FloorY = FloorReal32ToInt32(TestP.y);
		s32 CeilY = CeilReal32ToInt32(TestP.y) + 1;
        
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
        
		void* TextureMemory = Texture->Memory;
        
		r32 Inv255C = 1.0f / 255.0f;
		r32 One255C = 255.0f;
		__m128 Inv255C_x4 = _mm_set_ps1(Inv255C);
		__m128 One255C_x4 = _mm_set_ps1(One255C);
		__m128 Four_x4 = _mm_set_ps1(4.0f);
		__m128 StepFour_x4 = _mm_set_ps(3.0f, 2.0f, 1.0f, 0.0f);
		__m128 Min_x4 = _mm_set_ps1((r32)FillRect.MinX);
		__m128i MaskFF00FF = _mm_set1_epi32(0x00FF00FF);
		__m128i MaskFFFF = _mm_set1_epi32(0xFFFF);
		__m128 One = _mm_set_ps1(1.0f);
		__m128 Zero = _mm_set_ps1(0.0f);
		__m128 Half = _mm_set_ps1(0.5f);
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
        
		__m128 WidthM2 = _mm_set1_ps((r32)(Texture->Width - 2));
		__m128 HeightM2 = _mm_set1_ps((r32)(Texture->Height - 2));
        
		__m128 InvAxisXLengthSq_x4 = _mm_set_ps1(InvAxisXLengthSq);
		__m128 InvAxisYLengthSq_x4 = _mm_set_ps1(InvAxisYLengthSq);
        
		__m128 NAxisXx = _mm_mul_ps(XAxisx, InvAxisXLengthSq_x4);
		__m128 NAxisXy = _mm_mul_ps(XAxisy, InvAxisXLengthSq_x4);
		__m128 NAxisYx = _mm_mul_ps(YAxisx, InvAxisYLengthSq_x4);
		__m128 NAxisYy = _mm_mul_ps(YAxisy, InvAxisYLengthSq_x4);
        
        
		__m128 MaxColorValue = _mm_set1_ps(255.0f * 255.0f);
		__m128i TexturePitch_x4 = _mm_set1_epi32(Texture->Pitch);
		__m128i MaskFF = _mm_set1_epi32(0xFF);
		s32 RowAdvance = 2 * Buffer->Pitch;
        
		u8* Row = ((u8*)Buffer->Memory + FillRect.MinX * BITMAP_BYTE_PER_PIXEL + FillRect.MinY * Buffer->Pitch);
        
        TIMED_BLOCK(PixelFill, GetClampedRectArea(FillRect) / 2);
		for (s32 Y = FillRect.MinY; Y < FillRect.MaxY; Y += 2) {
			__m128 Y_x4 = _mm_set_ps1((r32)Y);
			__m128 Dy = _mm_sub_ps(Y_x4, OriginY_x4);
            
			__m128 PynX = _mm_mul_ps(Dy, NAxisXy);
			__m128 PynY = _mm_mul_ps(Dy, NAxisYy);
            
			__m128 tempPx = _mm_add_ps(Min_x4, StepFour_x4);
			tempPx = _mm_sub_ps(tempPx, OriginX_x4);
            
			__m128i ClipMask = StartClipMask;
            
			u32* Pixel = (u32*)Row;
            
			for (s32 X = FillRect.MinX; X < FillRect.MaxX; X += 4) {
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
                
				__m128 tX = _mm_add_ps(_mm_mul_ps(U, WidthM2), Half);
				__m128 tY = _mm_add_ps(_mm_mul_ps(V, HeightM2), Half);
                
				__m128i intX = _mm_cvttps_epi32(tX);
				__m128i intY = _mm_cvttps_epi32(tY);
                
				__m128 fX = _mm_sub_ps(tX, _mm_cvtepi32_ps(intX));
				__m128 fY = _mm_sub_ps(tY, _mm_cvtepi32_ps(intY));
#if 1	
				__m128i FetchX_4x = _mm_slli_epi32(intX, 2);
				__m128i FetchY_4x = _mm_or_si128(_mm_mullo_epi16(intY, TexturePitch_x4),
                                                 _mm_slli_epi32(_mm_mulhi_epi16(intY, TexturePitch_x4), 16));
                
				__m128i Fetch_4x = _mm_add_epi32(FetchX_4x, FetchY_4x);
                
				s32 Fetch0 = ((s32*)&(Fetch_4x))[0];
				s32 Fetch1 = ((s32*)&(Fetch_4x))[1];
				s32 Fetch2 = ((s32*)&(Fetch_4x))[2];
				s32 Fetch3 = ((s32*)&(Fetch_4x))[3];
                
				u8* TexelPtr0 = ((u8*)TextureMemory) + Fetch0;
				u8* TexelPtr1 = ((u8*)TextureMemory) + Fetch1;
				u8* TexelPtr2 = ((u8*)TextureMemory) + Fetch2;
				u8* TexelPtr3 = ((u8*)TextureMemory) + Fetch3;
                
                
				__m128i SampleA = _mm_setr_epi32(*(u32*)TexelPtr0, *(u32*)TexelPtr1, *(u32*)TexelPtr2, *(u32*)TexelPtr3);
				__m128i SampleB = _mm_setr_epi32(*(u32*)(TexelPtr0 + sizeof(u32)),
                                                 *(u32*)(TexelPtr1 + sizeof(u32)),
                                                 *(u32*)(TexelPtr2 + sizeof(u32)),
                                                 *(u32*)(TexelPtr3 + sizeof(u32)));
				__m128i SampleC = _mm_setr_epi32(*(u32*)(TexelPtr0 + Texture->Pitch),
                                                 *(u32*)(TexelPtr1 + Texture->Pitch),
                                                 *(u32*)(TexelPtr2 + Texture->Pitch),
                                                 *(u32*)(TexelPtr3 + Texture->Pitch));
				__m128i SampleD = _mm_setr_epi32(*(u32*)(TexelPtr0 + Texture->Pitch + sizeof(u32)),
                                                 *(u32*)(TexelPtr1 + Texture->Pitch + sizeof(u32)),
                                                 *(u32*)(TexelPtr2 + Texture->Pitch + sizeof(u32)),
                                                 *(u32*)(TexelPtr3 + Texture->Pitch + sizeof(u32)));
                
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
					s32 FetchX = ((u32*)&(intX))[I];
					s32 FetchY = ((u32*)&(intY))[I];
                    
					u8* TexelPtr = ((u8*)Texture->Memory) + Texture->Pitch * FetchY + FetchX * sizeof(u32);
					((u32*)&(SampleA))[I] = *(u32*)TexelPtr;
					((u32*)&(SampleB))[I] = *(u32*)(TexelPtr + sizeof(u32));
					((u32*)&(SampleC))[I] = *(u32*)(TexelPtr + Texture->Pitch);
					((u32*)&(SampleD))[I] = *(u32*)(TexelPtr + Texture->Pitch + sizeof(u32));
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
	}
}
