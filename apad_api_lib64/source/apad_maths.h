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
		ui16 x;
		ui16 width;
	};
	
	union {
		ui16 y;
		ui16 height;
	};
};
typedef point size;

#define 				Cap(_value, _min, _max) { if((_value) < (_min)) (_value) = (_min); \
																					else if((_value) > (_max)) (_value) = (_max); }
#define 				Magnitude(_x) ((_x) < 0 ? -(_x) : (_x))																				
dll_export size GetMiddle(rectangle r);
#define 				GetMin(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define 				GetMax(_a, _b) ((_a) > (_b) ? (_a) : (_b))
dll_import bool Overlap(ui16 x0, ui16 y0, ui16 left1, ui16 bottom1, ui16 width1, ui16 height1);

#endif