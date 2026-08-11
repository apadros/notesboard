#ifndef APAD_MATHS_H
#define APAD_MATHS_H

#include "apad_base_types.h"
#include "apad_intrinsics.h"

struct vector {
	union {
		f32 x;
		f32 width;
	};
	
	union {
		f32 y;
		f32 height;
	};
	
	dll_import vector operator+(vector& v);
	dll_import vector operator-(vector& v);
	dll_import void   operator+=(vector& v);
	dll_import void   operator-=(vector& v);
	dll_import vector operator*(f32 f);
	dll_import vector operator/(f32 f);
};

struct rectangle {
	union {
		struct {
			f32 left;
			f32 bottom;
		};
		vector pos;
	};
	
	union {
		struct {
			f32 width;
			f32 height;
		};
		vector size;
	};
};

dll_import f32 			 ArcCos(f32 f);
dll_import f32 			 ArcSine(f32 f);
dll_import f32 			 ArcTan(f32 f);
#define 				     Clamp(_value, _min, _max) { if((_value) < (_min)) (_value) = (_min); \
								     													   else if((_value) > (_max)) (_value) = (_max); }
dll_import f32 			 Cos(f32 degs);
dll_import rectangle CreateRectangle(f32 left, f32 bottom, f32 width, f32 height);
dll_export rectangle CreateRectangle(vector pos, vector size);
dll_import vector 	 CreateVector(f32 x, f32 y);
dll_import f32* 		 GenerateCircularCoords(ui8 count, f32 centerX, f32 centerY, f32 radius); // Coords will start at the 12 O'Clock position. Must be freed with Win32FreeMemory()
dll_import vector    GetCenter(rectangle r);
#define 				     GetMin(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define 				     GetMax(_a, _b) ((_a) > (_b) ? (_a) : (_b))
dll_import vector    GetTopRight(rectangle& r);
dll_import f32       LERP(f32 min, f32 max, f32 perc); // Linear interpolation
dll_export f32 			 Magnitude(f32 f);
dll_export f32 			 Magnitude(vector& v);
dll_import bool      Overlap(f32 x0, f32 y0, f32 left1, f32 bottom1, f32 width1, f32 height1); // Point-rectangle collision detection
dll_import bool 		 Overlap(f32 x0, f32 y0, f32 x1, f32 y1, f32 testDistance); // Circle-circle collision detection
dll_import f32       RoundToNearestInteger(f32 value);
dll_import f32 			 Sine(f32 degs);
dll_import f32 			 SquareRoot(f32 f);
dll_import f32 			 Tan(f32 degs);
#define              UnpackRectangle(_r) (_r).left, (_r).bottom, (_r).width, (_r).height
#define              UnpackVector(_v) (_v).x, (_v).y

#endif