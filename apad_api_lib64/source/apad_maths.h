#ifndef APAD_MATHS_H
#define APAD_MATHS_H

#include "apad_base_types.h"

struct rectangle {
	f32 left;
	f32 bottom;
	f32 width;
	f32 height;
};

struct point {
	union {
		f32 x;
		f32 width;
	};
	
	union {
		f32 y;
		f32 height;
	};
};
typedef point size;

#define 				Cap(_value, _min, _max) { if((_value) < (_min)) (_value) = (_min); \
																					else if((_value) > (_max)) (_value) = (_max); }
#define 				Magnitude(_x) ((_x) < 0 ? -(_x) : (_x))																				
dll_export size GetMiddle(rectangle r);
#define 				GetMin(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define 				GetMax(_a, _b) ((_a) > (_b) ? (_a) : (_b))
dll_import bool Overlap(f32 x0, f32 y0, f32 left1, f32 bottom1, f32 width1, f32 height1);

#endif