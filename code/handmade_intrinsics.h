#if !defined(HANDMADE_INTRINSICS_H)
#define HANDMADE_INTRINSICS_H
#include <math.h>



inline r32
SquareRoot(r32 Real32) {
	r32 Result = sqrtf(Real32);
	return(Result);
}


inline r32
AbsoluteValue(r32 Real32) {
	r32 Result = fabsf(Real32);
	return(Result);
}

inline u32
RotateLeft(u32 Value, s32 Amount) {
#if COMPILER_MSVC
	u32 Result = _rotl(Value, Amount);
#else
	Amount &= 31;
	u32 Result = ((Value << Amount) | (Value >> (32 - Amount)))
#endif
	return(Result);
}

inline u32
RotateRight(u32 Value, s32 Amount) {
#if COMPILER_MSVC
	u32 Result = _rotr(Value, Amount);
#else
	Amount &= 31;
	u32 Result = ((Value >> Amount) | (Value << (32 - Amount)))
#endif
	return(Result);
}

inline s32
RoundReal32ToInt32(r32 Real32) {
	s32 Result = (s32)roundf(Real32);
	return(Result);
}

inline u32
RoundReal32ToUInt32(r32 Real32) {
	u32 Result = (u32)roundf(Real32);
	return(Result);
}

inline s32
TruncateReal32ToInt32(r32 Real32) {
	s32 Result = (s32)(Real32);
	return(Result);
}

inline s32
FloorReal32ToInt32(r32 Real32) {
	s32 Result = (s32)floorf(Real32);
	return(Result);
}
inline u32
FloorReal32ToUInt32(r32 Real32) {
	u32 Result = (u32)floorf(Real32);
	return(Result);
}

inline s32
CeilReal32ToInt32(r32 Real32) {
	s32 Result = (s32)ceilf(Real32);
	return(Result);
}

inline r32
Sin(r32 Angle) {
	r32 Result = sinf(Angle);
	return(Result);
}

inline r32
Cos(r32 Angle) {
	r32 Result = cosf(Angle);
	return(Result);
}

inline r32
Atan2(r32 Y, r32 X) {
	r32 Result = atan2f(Y, X);
	return(Result);
}

struct bit_scan_result {
	b32 Found;
	u32 Index;
};
#pragma intrinsic(_BitScanForward)

inline bit_scan_result
FindLeastSignificantSetBit(u32 Value) {
	bit_scan_result Result = {};
    
#if COMPILER_MSVC
	Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
	for (u32 Test = 0; Test < 32; ++Test) {
		if (Value & (1 << Test)) {
			Result.Index = Test;
			Result.Found = true;
			break;
		}
	}
#endif
	return(Result);
}


#endif