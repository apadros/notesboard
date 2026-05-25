#ifndef APAD_ARRAY_H
#define APAD_ARRAY_H

#include "apad_memory.h"

// ********************************************************
// 
// This API will only work with statically-defined arrays!!
// 
// ********************************************************

#define AddToArray(_val, _ar, _uninitVal) { \
																						auto _length = GetArrayLength(_ar); \
																						bool _added = false; \
																						ForAll(_length) { \
																							if((_ar)[it] == (_uninitVal)) { \
																								(_ar)[it] = (_val); \
																								_added = true; \
																								break; \
																							} \
																						} \
																						Assert(_added == true); \
																					}

#define ClearArray(_ar) \
												ClearMemory(&(_ar), sizeof(_ar))
#define	GetArrayLength(_ar) \
														(sizeof(_ar) / sizeof((_ar)[0]))

#endif