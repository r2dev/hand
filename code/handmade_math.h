#if !defined(HANDMADE_MATH_H)
#define HANDMADE_MATH_H

struct v2 {
	union {
		struct {
			real32 X;
			real32 Y;
		};
		real32 E[2];
	};

	inline v2& operator*=(real32 A);
	inline v2& operator+=(v2 A);
};

inline v2
V2(real32 X, real32 Y) {
	v2 Result;
	Result.X = X;
	Result.Y = Y;
	return Result;
}

inline v2
operator-(v2 A) {
	v2 Result;
	Result.X = -A.X;
	Result.Y = -A.Y;
	return (Result);
}

inline v2
operator+(v2 A, v2 B) {
	v2 Result;
	Result.X = A.X + B.X;
	Result.Y = A.Y + B.Y;
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
	Result.X = A.X - B.X;
	Result.Y = A.Y - B.Y;
	return Result;
}

inline v2
operator*(real32 A, v2 B) {
	v2 Result;
	Result.X = A * B.X;
	Result.Y = A * B.Y;
	return (Result);
}
inline v2
operator*(v2 B, real32 A) {
	v2 Result;
	Result.X = A * B.X;
	Result.Y = A * B.Y;
	return (Result);
}

inline v2&
v2::operator*=(real32 A) {
	*this = A * *this;
	return (*this);
}

inline real32
Square(real32 A) {
	real32 Result = A * A;
	return (Result);
}

inline real32
Inner(v2 A, v2 B) {
	real32 Result = A.X * B.X + A.Y * B.Y;
	return(Result);
}

inline real32
LengthSq(v2 A) {
	real32 Result = Inner(A, A);
	return (Result);
}

inline int32
SignOf(int32 Value) {
	int32 Result = (Value >= 0) ? 1 : -1;
	return(Result);
}

struct rectangle2 {
	v2 Min;
	v2 Max;
};

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


inline bool32
IsInRectangle(rectangle2 Rectangle, v2 Test) {
	bool32 Result = ((Test.X >= Rectangle.Min.X)
		&& (Test.Y >= Rectangle.Min.Y)
		&& (Test.X < Rectangle.Max.X)
		&& (Test.X < Rectangle.Max.Y));
	return(Result);
}

#endif