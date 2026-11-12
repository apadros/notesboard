#ifndef APAD_LOGGING_H
#define APAD_LOGGING_H

#include "apad_base_types.h"
#include "apad_intrinsics.h"
#include "apad_memory.h"
#include "apad_string_args.h"

typedef memory_stack log_file;

#define NullLogFile NullMemoryBlock

dll_import void 		CloseLogFile(log_file&);
dll_import void* 		GetMemory(log_file); // GetMemory(memory_stack) in apad_memory.cpp
dll_import bool 		IsValid(log_file&); // IsValid(memory_stack&) in apad_memory.cpp
dll_import log_file OpenLogFile();
dll_import void 		Log( log_file&, 
												 string_args(formatString),
												 // Format strings
												 // %s (string literal), 
												 // %si8, %ui8, %si16, %ui16, %si32, %ui32, %si64, %ui64, 
												 // %f32, %f64
												 // Floating point precision limited to 2 decimal points
												 // Need to use standard C standard library escape sequences for certain characters, e.g. \" for quotation marks
												 ... );

#endif