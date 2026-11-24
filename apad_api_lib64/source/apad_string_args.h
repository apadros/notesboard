#ifndef APAD_STRING_ARGS_H
#define APAD_STRING_ARGS_H

#include "apad_base_types.h"

// Expands to _id, _idLength. The latter can be set to Null to scan until the EOS char
#define string_args(_id) char* _id, \
												 ui16  _id ## Length
#define stringArgsLength(_id) _id ## Length

dll_export void CheckStringArgs( // Call this at the start of the function before using the string arguments
                                 char*& string, 
                                 ui16&  length); // If length == 0, will assume null-terminated string and will set length with GetLength()
#define         NullStringArgs Null, Null
#define         UnpackStringArgs(_id) _id, _id ## Length

#endif