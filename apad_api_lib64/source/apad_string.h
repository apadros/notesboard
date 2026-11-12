#ifndef APAD_STRING_H
#define APAD_STRING_H

#include "apad_base_types.h"
#include "apad_intrinsics.h"
#include "apad_memory.h"
#include "apad_string_args.h"

// ******************** Conversions ******************** //

dll_import char* ConvertStringToLowerCase(string_args(string)); // Returns contains an EOS char at the end and must be freed with Free(char*)

// All ToString() functions return a string allocated on global API memory.
dll_import char* ToString(si8 i);
dll_import char* ToString(ui8 i);
dll_import char* ToString(si16 i);
dll_import char* ToString(ui16 i);
dll_import char* ToString(si32 i);
dll_import char* ToString(ui32 i);
dll_import char* ToString(si64 i);
dll_import char* ToString(ui64 i);
dll_import char* ToString(f32 f); // Return limited to 2 decimal places with rounding
dll_import char* ToString(f64 f); // Return limited to 2 decimal places with rounding
dll_import si32  StringToInt(string_args(string));

// ******************** Others ******************** //

dll_import bool IsLetter(char c);
dll_import bool IsWord(string_args(string));
dll_import bool IsNumber(char c);
dll_import bool IsNumber(string_args(string));
dll_import bool IsWhitespace(char c); // Space, horizontal & vertical tabs, carriage return, newline & feed

dll_import char* AllocateString(string_args(string)); // Return contains an EOS char at the end and must be freed with Free(char*)
dll_import bool  AreEqual(string_args(s1), string_args(s2));
dll_import char* // Contains a null char at the end. Must be freed with Free(char*)
								 Concatenate( // Will remove all null-chars from all strings supplied and automatically add one to the final returned string
															ui8 count, 
															...); // All args must be char*
dll_import bool  ContainsAnySubstring(string_args(string), char** substrings, ui8 subsCount); 
dll_import void  Copy(string_args(source), string_args(destination));
dll_import char* // Contains a null char at the end. Must be freed with Free(void*)
					       ExtractSubstring(char* string, 
																	ui16 	length); // Set to Null to extract until the EOS char. If this is larger than the actual string length, extraction will stop after the EOS char
dll_import char* FindSubstring(string_args(sub), string_args(string)); // Will return the start of the substring within string, including everything after the former. Does not need to be freed.
dll_import void  Free(void* string); // Only for strings allocated through this- or the memroy- APIs. Points to Free(void*) in apad_memory.cpp.
dll_import ui16  // Will == Null if no EOS char is found within 300 chars from string start
								 // If not == Null, will not include the EOS
								 GetLength(char* s);
dll_import char* Push(string_args(string), bool addEOS, memory_stack& stack); // If only an EOS char is wanted, set string & length to Null and addEOS to true.
dll_import bool  StringIsEqualToAny(string_args(string), char** strings, ui8 count);

#endif