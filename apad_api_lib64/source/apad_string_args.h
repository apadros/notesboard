#ifndef APAD_STRING_ARGS_H
#define APAD_STRING_ARGS_H

// Expands to _id, _idLength. The latter can be set to Null to scan until the EOS char
#define string_args(_id) char* _id, \
												 ui16  _id ## Length
#define stringArgsLength(_id) _id ## Length
#define NullStringArgs Null, Null
#define UnpackStringArgs(_id) _id, _id ## Length

#endif