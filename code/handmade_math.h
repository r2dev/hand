#if !defined(HANDMADE_MATH_H)
#define HANDMADE_MATH_H
#include <limits.h>
struct v2 {
	union {
		struct {
			r32 x;
			r32 y;
		};
		r32 E[2];
	};
    
	inline v2& operator*=(r32 A);
	inline v2& operator+=(v2 A);
    inline v2& operator-=(v2 A);
};

inline v2
V2(r32 X, r32 Y) {
	v2 Result;
	Result.x = X;
	Result.y = Y;
	return Result;
}

struct v3 {
	union {
		struct {
			r32 x, y, z;
		};
		struct {
			r32 r, g, b;
		};
		struct {
			v2 xy;
			r32 Ignored0_;
		};
		r32 E[3];
	};
	inline v3& operator*=(r32 A);
	inline v3& operator+=(v3 A);
    inline v3& operator-=(v3 A);
};

Introspect(category: "math") struct rectangle3 {
	v3 Min;
	v3 Max;
};
Introspect(category: "math") struct rectangle2 {
	v2 Min;
	v2 Max;
};

struct rectangle2i {
	s32 MinX;
	s32 MinY;
	s32 MaxX;
	s32 MaxY;
};

inline v2
V2(v3 A) {
	v2 Result;
	Result.x = A.x;
	Result.y = A.y;
	return Result;
}

inline v3 V3(v2 XY, r32 Z) {
	v3 Result;
	Result.x = XY.x;
	Result.y = XY.y;
	Result.z = Z;
	return(Result);
}

inline v3
V3(r32 X, r32 Y, r32 Z) {
	v3 Result;
	Result.x = X;
	Result.y = Y;
	Result.z = Z;
	return Result;
}

//v4
struct v4 {
	union {
		struct {
			union {
				v3 xyz;
				struct {
					r32 x, y, z;	
				};
			};
			r32 w;
		};
		struct {
			union {
				v3 rgb;
				struct {
					r32 r, g, b;
				};
			};
			r32 a;
		};
		struct {
			v2 xy;
			r32 Ignored0_;
			r32 Ignored1_;
		};
		struct {
			r32 Ignored2_;
			v2 yz;
			r32 Ignored3_;
		};
		struct {
			r32 Ignored4_;
			r32 Ignored5_;
			v2 zw;
			
		};
		r32 E[4];
	};
    
	inline v4& operator*=(r32 A);
	inline v4& operator+=(v4 A);
};



inline v4
V4(r32 X, r32 Y, r32 Z, r32 W) {
	v4 Result;
	Result.x = X;
	Result.y = Y;
	Result.z = Z;
	Result.w = W;
	return Result;
}

inline v4
V4(v3 XYZ, r32 W) {
	v4 Result;
	Result.xyz = XYZ;
	Result.w = W;
	return Result;
}

inline v4
operator-(v4 A) {
	v4 Result;
	Result.x = -A.x;
	Result.y = -A.y;
	Result.z = -A.z;
	Result.w = -A.w;
	return (Result);
}

inline v4
operator+(v4 A, v4 B) {
	v4 Result;
	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;
	Result.w = A.w + B.w;
	return (Result);
}

inline v4&
v4::operator+=(v4 A) {
	*this = *this + A;
	return (*this);
}

inline v4
operator-(v4 A, v4 B) {
	v4 Result;
	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;
	Result.w = A.w - B.w;
	return Result;
}

inline v4
operator*(r32 A, v4 B) {
	v4 Result;
	Result.x = A * B.x;
	Result.y = A * B.y;
	Result.z = A * B.z;
	Result.w = A * B.w;
	return (Result);
}
inline v4
operator*(v4 B, r32 A) {
	v4 Result;
	Result.x = A * B.x;
	Result.y = A * B.y;
	Result.z = A * B.z;
	Result.w = A * B.w;
	return (Result);
}

inline v4&
v4::operator*=(r32 A) {
	*this = A * *this;
	return (*this);
}
inline v4
Hadamard(v4 A, v4 B) {
	v4 Result = { A.x * B.x, A.y * B.y, A.z * B.z, A.w * B.w };
	return(Result);
    
}

inline r32
Inner(v4 A, v4 B) {
	r32 Result = A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w;
	return(Result);
}

inline r32
LengthSq(v4 A) {
	r32 Result = Inner(A, A);
	return (Result);
}

inline r32
Length(v4 A) {
	r32 Result = SquareRoot(LengthSq(A));
	return(Result);
}

inline v4
Lerp(v4 A, r32 t, v4 B) {
	v4 Result = {};
	Result.r = A.r * (1.0f - t) + B.r * t;
	Result.g = A.g * (1.0f - t) + B.g * t;
	Result.b = A.b * (1.0f - t) + B.b * t;
	Result.a = A.a * (1.0f - t) + B.a * t;
	return Result;
}

inline v4
ToV4(v3 A, r32 V) {
	v4 Result;
	Result.xyz = A;
	Result.w = V;
	return(Result);
}

inline v3
ToV3(v2 A, r32 V) {
	v3 Result;
	Result.xy = A;
	Result.z = V;
	return(Result);
}

inline r32
Square(r32 A) {
	r32 Result = A * A;
	return (Result);
}

inline r32
Lerp(r32 A, r32 t, r32 B) {
	r32 Result = A * (1.0f - t) + B * t;
	return(Result);
}



inline r32
Clamp(r32 Min, r32 Value, r32 Max) {
	r32 Result = Value;
	if (Result < Min) {
		Result = Min;
	}
	if (Result > Max) {
		Result = Max;
	}
    
	return(Result);
}

inline v2
Perp(v2 A) {
	v2 Result = {-A.y, A.x};
	return(Result);
}

inline r32
Clamp01(r32 Value) {
	r32 Result = Clamp(0.0f, Value, 1.0f);
	return(Result);
}

inline v3
Clamp01(v3 Value) {
	v3 Result;
	Result.x = Clamp01(Value.x);
	Result.y = Clamp01(Value.y);
	Result.z = Clamp01(Value.z);
	return(Result);
}

inline r32 
Clamp01MapToRange(r32 Min, r32 Value, r32 Max) {
	r32 Result = 0.0f;
	r32 Range = (Max - Min);
	if (Range != 0.0f) {
		Result = Clamp01((Value - Min) / Range);
	}
    
	return(Result);
}



inline v2
Clamp01(v2 Value) {
	v2 Result;
	Result.x = Clamp01(Value.x);
	Result.y = Clamp01(Value.y);
	return(Result);
}
////

inline v2
operator-(v2 A) {
	v2 Result;
	Result.x = -A.x;
	Result.y = -A.y;
	return (Result);
}

inline v2
operator+(v2 A, v2 B) {
	v2 Result;
	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	return (Result);
}

inline v2&
v2::operator+=(v2 A) {
	*this = *this + A;
	return (*this);
}

inline v2
operator-(v2 A, v2 B) {
	v2 Result;
	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	return Result;
}

inline v2&
v2::operator-=(v2 A) {
	*this = *this - A;
	return (*this);
}

inline v2
operator*(r32 A, v2 B) {
	v2 Result;
	Result.x = A * B.x;
	Result.y = A * B.y;
	return (Result);
}
inline v2
operator*(v2 B, r32 A) {
	v2 Result;
	Result.x = A * B.x;
	Result.y = A * B.y;
	return (Result);
}

inline v2&
v2::operator*=(r32 A) {
	*this = A * *this;
	return (*this);
}

inline v2
V2i(s32 X, s32 Y) {
	v2 Result = { (r32)X, (r32)Y };
	return(Result);
}

inline v2
V2i(u32 X, u32 Y) {
	v2 Result = { (r32)X, (r32)Y };
	return(Result);
}



inline v2
Hadamard(v2 A, v2 B) {
	v2 Result = {A.x * B.x, A.y * B.y};
	return(Result);
    
}

inline r32
Inner(v2 A, v2 B) {
	r32 Result = A.x * B.x + A.y * B.y;
	return(Result);
}

inline r32
LengthSq(v2 A) {
	r32 Result = Inner(A, A);
	return (Result);
}

inline r32
Length(v2 A) {
	r32 Result = SquareRoot(LengthSq(A));
	return(Result);
}


////

inline v3
operator-(v3 A) {
	v3 Result;
	Result.x = -A.x;
	Result.y = -A.y;
	Result.z = -A.z;
	return (Result);
}

inline v3
operator+(v3 A, v3 B) {
	v3 Result;
	Result.x = A.x + B.x;
	Result.y = A.y + B.y;
	Result.z = A.z + B.z;
	return (Result);
}

inline v3&
v3::operator+=(v3 A) {
	*this = *this + A;
	return (*this);
}

inline v3
operator-(v3 A, v3 B) {
	v3 Result;
	Result.x = A.x - B.x;
	Result.y = A.y - B.y;
	Result.z = A.z - B.z;
	return Result;
}

inline v3&
v3::operator-=(v3 A) {
	*this = *this - A;
	return (*this);
}

inline v3
operator*(r32 A, v3 B) {
	v3 Result;
	Result.x = A * B.x;
	Result.y = A * B.y;
	Result.z = A * B.z;
	return (Result);
}
inline v3
operator*(v3 B, r32 A) {
	v3 Result;
	Result.x = A * B.x;
	Result.y = A * B.y;
	Result.z = A * B.z;
	return (Result);
}

inline v3&
v3::operator*=(r32 A) {
	*this = A * *this;
	return (*this);
}

inline v3
Hadamard(v3 A, v3 B) {
	v3 Result = { A.x * B.x, A.y * B.y, A.z * B.z };
	return(Result);
    
}

inline r32
Inner(v3 A, v3 B) {
	r32 Result = A.x * B.x + A.y * B.y + A.z * B.z;
	return(Result);
}

inline r32
LengthSq(v3 A) {
	r32 Result = Inner(A, A);
	return (Result);
}

inline r32
Length(v3 A) {
	r32 Result = SquareRoot(LengthSq(A));
	return(Result);
}

inline v3
Normalize(v3 A) {
	r32 InvLength = 1.0f / Length(A);
	v3 Result = A * InvLength;
	return(Result);
}


inline v3
Lerp(v3 A, r32 t, v3 B) {
	v3 Result = {};
	Result.r = A.r * (1.0f - t) + B.r * t;
	Result.g = A.g * (1.0f - t) + B.g * t;
	Result.b = A.b * (1.0f - t) + B.b * t;
	return Result;
}



inline s32
SignOf(s32 Value) {
	s32 Result = (Value >= 0) ? 1 : -1;
	return(Result);
}

inline r32
SignOf(r32 Value) {
	r32 Result = (Value >= 0) ? 1.0f : -1.0f;
	return(Result);
}


//rect 3
inline v3
GetMinCorner(rectangle3 Rect) {
	v3 Result = Rect.Min;
	return(Result);
}

inline v3
GetMaxCorner(rectangle3 Rect) {
	v3 Result = Rect.Max;
	return(Result);
}

inline v3
GetCenter(rectangle3 Rect) {
	v3 Result = 0.5f * (Rect.Min + Rect.Max);
	return(Result);
}

inline v2
GetDim(rectangle2 Rect) {
	v2 Result = Rect.Max - Rect.Min;
	return(Result);
}

inline v3
GetDim(rectangle3 Rect) {
	v3 Result = Rect.Max - Rect.Min;
	return(Result);
}

inline rectangle3
RectMinMax(v3 Min, v3 Max) {
	rectangle3 Result = {};
	Result.Min = Min;
	Result.Max = Max;
	return(Result);
}

inline rectangle3
RectMinDim(v3 Min, v3 Dim) {
	rectangle3 Result = {};
	Result.Min = Min;
	Result.Max = Min + Dim;
	return(Result);
}


inline rectangle3
RectCenterHalfDim(v3 Center, v3 Dim) {
	rectangle3 Result = {};
	Result.Min = Center - Dim;
	Result.Max = Center + Dim;
	return(Result);
}

inline rectangle3
RectCenterDim(v3 Center, v3 Dim) {
	rectangle3 Result = RectCenterHalfDim(Center, 0.5f * Dim);
	return(Result);
}
inline rectangle3
AddRadiusTo(rectangle3 A, v3 Radius) {
	rectangle3 Result = {};
	Result.Min = A.Min - Radius;
	Result.Max = A.Max + Radius;
	return(Result);
}


inline b32
IsInRectangle(rectangle3 Rectangle, v3 Test) {
	b32 Result = ((Test.x >= Rectangle.Min.x)
                  && (Test.y >= Rectangle.Min.y)
                  && (Test.z >= Rectangle.Min.z)
                  && (Test.x < Rectangle.Max.x)
                  && (Test.y < Rectangle.Max.y)
                  && (Test.z < Rectangle.Max.z)
                  );
	return(Result);
}

inline b32
RectanglesIntersect(rectangle3 A, rectangle3 B) {
	b32 Result = !((A.Min.x > B.Max.x)
                   || (B.Min.x >= A.Max.x)
                   || (A.Min.y >= B.Max.y)
                   || (B.Min.y >= A.Max.y)
                   || (A.Min.z >= B.Max.z)
                   || (B.Min.z >= A.Max.z));
	return(Result);
}

//rect 2

inline v2
GetMinCorner(rectangle2 Rect) {
	v2 Result = Rect.Min;
	return(Result);
}

inline v2
GetMaxCorner(rectangle2 Rect) {
	v2 Result = Rect.Max;
	return(Result);
}

inline v2
GetCenter(rectangle2 Rect) {
	v2 Result = 0.5f * (Rect.Min + Rect.Max);
	return(Result);
}

inline v2
Arm2(r32 Angle) {
    v2 Result = {Cos(Angle), Sin(Angle)};
    return(Result);
}

inline rectangle2
InvertedInfinityRectangle2() {
	rectangle2 Result;
	Result.Min.x = Result.Min.y = Real32Maximum;
	Result.Max.x = Result.Max.y = -Real32Maximum;
	return(Result);
}

inline rectangle2
Union(rectangle2 A, rectangle2 B) {
    rectangle2 Result;
    Result.Min.x = (A.Min.x < B.Min.x)? A.Min.x: B.Min.x;
    Result.Min.y = (A.Min.y < B.Min.y)? A.Min.y: B.Min.y;
    
    Result.Max.x = (A.Max.x > B.Max.x)? A.Max.x: B.Max.x;
    Result.Max.y = (A.Max.y > B.Max.y)? A.Max.y: B.Max.y;
    return(Result);
}

inline rectangle2
RectMinMax(v2 Min, v2 Max) {
	rectangle2 Result = {};
	Result.Min = Min;
	Result.Max = Max;
	return(Result);
}

inline rectangle2
RectMinDim(v2 Min, v2 Dim) {
	rectangle2 Result = {};
	Result.Min = Min;
	Result.Max = Min + Dim;
	return(Result);
}


inline rectangle2
RectCenterHalfDim(v2 Center, v2 Dim) {
	rectangle2 Result = {};
	Result.Min = Center - Dim;
	Result.Max = Center + Dim;
	return(Result);
}

inline rectangle2
RectCenterDim(v2 Center, v2 Dim) {
	rectangle2 Result = RectCenterHalfDim(Center, 0.5f * Dim);
	return(Result);
}

inline rectangle2
AddRadiusTo(rectangle2 A, v2 Radius) {
	rectangle2 Result = {};
	Result.Min = A.Min - Radius;
	Result.Max = A.Max + Radius;
	return(Result);
}

inline rectangle2
Offset(rectangle2 A, v2 P) {
    rectangle2 Result;
    Result.Min = A.Min + P;
    Result.Max = A.Max + P;
    return(Result);
}


inline b32
IsInRectangle(rectangle2 Rectangle, v2 Test) {
	b32 Result = ((Test.x >= Rectangle.Min.x)
                  && (Test.y >= Rectangle.Min.y)
                  && (Test.x < Rectangle.Max.x)
                  && (Test.y < Rectangle.Max.y));
	return(Result);
}

inline r32
SafeRatioN(r32 Numerator, r32 Divisor, r32 N) {
	r32 Result = N;
	if (Divisor != 0.0f) {
		Result = Numerator / Divisor;
	}
	return(Result);
}
inline r32
SafeRatio0(r32 Numerator, r32 Divisor) {
	return SafeRatioN(Numerator, Divisor, 0.0f);
}

inline r32
SafeRatio1(r32 Numerator, r32 Divisor) {
	return SafeRatioN(Numerator, Divisor, 1.0f);
}

inline v3
GetBarycentric(rectangle3 A, v3 P) {
	v3 Result;
	Result.x = SafeRatio0(P.x - A.Min.x, A.Max.x - A.Min.x);
	Result.y = SafeRatio0(P.y - A.Min.y, A.Max.y - A.Min.y);
	Result.z = SafeRatio0(P.z - A.Min.z, A.Max.z - A.Min.z);
	return(Result);
}

inline v2
GetBarycentric(rectangle2 A, v2 P) {
	v2 Result;
	Result.x = SafeRatio0(P.x - A.Min.x, A.Max.x - A.Min.x);
	Result.y = SafeRatio0(P.y - A.Min.y, A.Max.y - A.Min.y);
	return(Result);
}

inline rectangle2
ToRectangleXY(rectangle3 A) {
	rectangle2 Result;
	Result.Min = A.Min.xy;
	Result.Max = A.Max.xy;
	return(Result);
}

inline rectangle2i
InvertedInfinityRectangle2i() {
	rectangle2i Result;
	Result.MinX = Result.MinY = INT_MAX;
	Result.MaxX = Result.MaxY = -INT_MAX;
	return(Result);
}

inline rectangle2i
Intersect(rectangle2i A, rectangle2i B) {
	rectangle2i Result;
	Result.MinX = A.MinX < B.MinX ? B.MinX: A.MinX;
	Result.MinY = A.MinY < B.MinY ? B.MinY: A.MinY;
	Result.MaxX = A.MaxX > B.MaxX ? B.MaxX: A.MaxX;
	Result.MaxY = A.MaxY > B.MaxY ? B.MaxY: A.MaxY;
	return(Result);
}

inline b32
HasArea(rectangle2i A) {
	b32 Result = (A.MinX < A.MaxX) && (A.MinY < A.MaxY);
	return(Result);
}


internal v4
SRGBToLinear1(v4 C) {
	v4 Result = {};
	r32 Inv255C = 1.0f / 255.0f;
	Result.r = Square(Inv255C * C.r);
	Result.g = Square(Inv255C * C.g);
	Result.b = Square(Inv255C * C.b);
	Result.a = Inv255C * C.a;
	return(Result);
}


internal v4
Linear1ToSRGB(v4 C) {
	v4 Result = {};
	r32 One255C = 255.0f;
	Result.r = One255C * SquareRoot(C.r);
	Result.g = One255C * SquareRoot(C.g);
	Result.b = One255C * SquareRoot(C.b);
	Result.a = One255C * C.a;
	return(Result);
}

inline s32
GetClampedRectArea(rectangle2i A) {
    s32 Width = (A.MaxX - A.MinX);
    s32 Height = (A.MaxY - A.MinY);
    s32 Result = 0;
    if (Width > 0 && Height > 0) {
        Result = Width * Height;
    }
    return(Result);
}


inline void
DEBUGValueSetEventData(debug_event *Event, v2 Value) {
    Event->Type = DebugEvent_V2;
    Event->VecR32[0] = Value.x;
    Event->VecR32[1] = Value.y;
}
inline void
DEBUGValueSetEventData(debug_event *Event, v3 Value) {
    Event->Type = DebugEvent_V3;
    Event->VecR32[0] = Value.x;
    Event->VecR32[1] = Value.y;
    Event->VecR32[2] = Value.z;
}

inline void
DEBUGValueSetEventData(debug_event *Event, v4 Value) {
    Event->Type = DebugEvent_V4;
    Event->VecR32[0] = Value.x;
    Event->VecR32[1] = Value.y;
    Event->VecR32[2] = Value.z;
    Event->VecR32[3] = Value.w;
}

inline void
DEBUGValueSetEventData(debug_event *Event, rectangle2 Value) {
    Event->Type = DebugEvent_Rectangle2;
    Event->VecR32[0] = Value.Min.x;
    Event->VecR32[1] = Value.Min.y;
    Event->VecR32[2] = Value.Max.x;
    Event->VecR32[3] = Value.Max.y;
}

inline void
DEBUGValueSetEventData(debug_event *Event, rectangle3 Value) {
    Event->Type = DebugEvent_Rectangle3;
    Event->VecR32[0] = Value.Min.x;
    Event->VecR32[1] = Value.Min.y;
    Event->VecR32[2] = Value.Min.z;
    Event->VecR32[3] = Value.Max.x;
    Event->VecR32[4] = Value.Max.y;
    Event->VecR32[5] = Value.Max.z;
}




#endif
