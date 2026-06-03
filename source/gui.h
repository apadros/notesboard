#ifndef TEXT_H
#define TEXT_H

#include "apad_intrinsics.h."

// @TODO - Return a rectagle - pull out of win32_gui.h

program_external ui16 WriteText(const char* string, ui16 x, ui16 y, ui8 height);

#endif