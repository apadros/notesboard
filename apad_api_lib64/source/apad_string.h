#ifndef APAD_STRING_H
#define APAD_STRING_H

#include "apad_base_types.h"
#include "apad_intrinsics.h"
#include "apad_memory.h"

// ******************** Conversions ******************** //

dll_import void 	ConvertStringToLowerCase(const char* s);

// All ToString() functions return a string allocated on global API memory.
dll_import char* ToString(si8 i);
dll_import char* ToString(ui8 i);
dll_import char* ToString(si16 i);
dll_import char* ToString(ui16 i);
dll_import char* ToString(si32 i);
dll_import char* ToString(ui32 i);
dll_import char* ToString(si64 i);
dll_import char* ToString(ui64 i);
dll_import char* ToString(f32 f); // Return limited to 2 decimal places without rounding
dll_import char* ToString(f64 f); // Return limited to 2 decimal places without rounding
dll_import si32  StringToInt(const char* s,
														 ui16        length); // Set to Null to convert up to the null-char, must be supplied if string doesn't have one.

// ******************** Others ******************** //

dll_import bool IsLetter(char c);
dll_import bool IsWord(char* string);
dll_import bool IsNumber(char c);
dll_import bool IsNumber(char* string);

dll_import bool IsWhitespace(char c); // Space, horizontal & vertical tabs, carriage return, newline & feed

											 
											 
dll_import 			 char* AllocateString( // Allocates string on API global memory
																			 // Will automatically add a null-char if target length does not contain one
																			const char* s, 
																			ui16        length = Null); // Leave as Null to copy until and including the null-char
dll_import 			 char* Concatenate( // Allocates string on API global memory
																		// Will remove all null-chars from all strings supplied and automatically add one to the final returned string
																		ui8 count, 
																		...); // All args must be char*
dll_import 			 bool  ContainsAnySubstring(const char*  string, 
																						const char** substrings, 
																						ui8 				 subsLength);
dll_import 			 void  CopyString(const char* source, 
																	si16        srcLength, // Set to -1 to copy entire source including the null-character
																	const char* destination, 
																	ui16 			  destLength);
dll_import 			 char* ExtractSubstring( // Allocates a copy on API global memory
																				const char* string, 
																				ui16 				length); // Set to Null to extract until the null-character. If this is larger than the actual string length, extraction will stop after the null-character
dll_import const char* FindSubstring(const char* sub, 
																		 const char* string);
dll_import 			 void  FreeString(char* string); // Only for strings allocated on API global memory
dll_import 			 ui16  GetStringLength(const char* s); // Will return the length wihtout the null-character
dll_import 			 char* PushString( // If only a \0 is wanted, set string to Null and addEOS to true.
																	const char* 	string, 
																	bool 				  addEOS, 
																	memory_block& stack);
dll_import 			 bool  StringIsEqualToAny(const char*  string, 
																					const char** strings, 
																					ui8 				 count);
dll_import 			 bool  StringsAreEqual(const char* s1, 
																			 const char* s2);

#endif