#ifndef APAD_LOGGING_H
#define APAD_LOGGING_H

#include "apad_base_types.h"
#include "apad_intrinsics.h"
#include "apad_memory.h"

typedef memory_block log_file;

#define NullLogFile NullMemoryBlock

dll_import log_file OpenLogFile();
													 // Format strings
													 // %s (string literal), 
													 // %si8, %ui8, %si16, %ui16, %si32, %ui32, %si64, %ui64, 
													 // %f32, %f64
													 // Floating point precision limited to 2 decimal points
													 // Need to use standard C standard library escape sequences for certain characters, e.g. \" for quotation marks
dll_import void 		 Log(log_file& file, const char* formatString, ... );
dll_import void 		 CloseLogFile(log_file& log);

#endif