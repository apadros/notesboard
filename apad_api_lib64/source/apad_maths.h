#ifndef APAD_MATHS_H
#define APAD_MATHS_H

#define Cap(_value, _min, _max) { if((_value) < (_min)) (_value) = (_min); \
																	else if((_value) > (_max)) (_value) = (_max); }
#define Magnitude(_x) ((_x) < 0 ? -(_x) : (_x))
#define Min(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define Max(_a, _b) ((_a) > (_b) ? (_a) : (_b))

bool 		Overlap(ui16 x0, ui16 y0, ui16 left1, ui16 bottom1, ui16 width1, ui16 height1);

#endif