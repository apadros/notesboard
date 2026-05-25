#ifndef APAD_INTRINSICS_H
#define APAD_INTRINSICS_H

#include "apad_base_types.h"

// Storage and linkage modifiers
// Storage refers to data duration within the program (e.g block scope, until program termination etc).
// Linkage refers to wether data is accessible or has already been declared elsewhere (e.g. other translation units).
// Data can also have no linkage whatsoever, meaning it can only be referred to in its current scope.

				// Local variable or translation unit variable with program duration.
#define program_local static
				// Program duration, external linkage.
				// Can only be used in global scope
				// Need to be initialised in a translation unit, otherwise there'll be a multiple symbols error.
				// Previously this was extern inline... difference?
#define program_external extern  								
				// To enable a single definition to be copied to several translation units (e.g. within a header file).
        // Can only be used in global scope.
				// Requires at least c++17
#define program_unique inline 
				// For struct members, single member shared across struct instances.
        // Will exist even if not variables of that struct are instantiated.
#define shared_member static 
				// Allows a struct variable or function to be initialised directly inside the struct, no need for external code.
#define inited_member inline static 

// Looping
#define For for
#define ForAll(count) for(ui32 it = 0; it < (count); it++)
#define ForAllInc(count, increment) for(ui32 it = 0; it < (count); it += increment)
#define FromTo(start, end) for(ui32 it = (start); \
		                       ((end) - (start) > 0) ? it < (end) : it > end; \
		                       ((end) - (start) > 0) ? it++ : it--)
#define FromToInc(start, end) for(ui32 it = (start); \
		                          ((end) - (start) > 0) ? it <= (end) : it >= (end); \
		                          ((end) - (start) > 0) ? it++ : it--)

// Enums
// Format: BeginEnum(id) { options } EndEnum(id);
#define BeginEnum(_id) struct _id { enum : ui8 
#define EndEnum(_id) ; private: ui8 _value; \
				     public: void operator=(ui8 c) { _value = c; } \
                     b8 operator==(ui8 c) { return _value == c; } \
				     void operator+=(ui8 c) { _value += 1; } \
				     void operator-=(ui8 c) { _value -= 1; } \
				     b8 operator>(ui8 c) { return _value > c; } \
				     b8 operator<(ui8 c) { return _value < c; } \
				     operator ui8() { return _value; } \
					 _id() { _value = 0; } \
				     _id(ui8 c) { _value = c; } \
				     }
#define forward_declared_enum struct

// Bit fields. 
// Dot not test for true or false, instead use > 0 or == 0
#define BitIsSet(_bit, _field) ((ui32)(_field) & (ui32)(_bit))
#define ClearBit(_bit, _field) (_field = decltype(_field)((ui32)(_field) & (~(ui32)(_bit))))
#define SetBit(_bit, _field) (_field = decltype(_field)((ui32)(_field) | (ui32)(_bit)))

// Linking
#define dll_import __declspec( dllimport )
#define dll_export __declspec( dllexport )

// Others
#define Toggle(_data) (_data) = !(_data)
#define SetIfUnset(_data) if((_data) == false) \
                            (_data) = true
#define SizeOf sizeof

#endif
