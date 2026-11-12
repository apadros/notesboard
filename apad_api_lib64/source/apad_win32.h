#ifndef APAD_WIN32_H
#define APAD_WIN32_H

#include "apad_base_types.h"
#include "apad_intrinsics.h"
#include "apad_memory.h"
#include "apad_string_args.h"

#define ConsoleAppEntryPoint(_argumentsID, _argumentCountID) /* The first argument corresponds to the program name */ \
				  int main(int _argumentCountID, char** _argumentsID)

dll_import void Win32OutputDebugString(string_args(string)); 
dll_import void Win32PrintStackBackTrace(); // Need to compile without optimizations and generate debug info for this to be useful

// ******************** Memory ********************  //

dll_import void* // Will automatically be cleared.
								 Win32AllocateMemory( // Will trigger internal assertion if return not valid
																		  ui32 size);
dll_import void  Win32FreeMemory( // Will trigger internal assertion if something goes wrong
																  void* mem);

// ******************** Files ********************  //

dll_import void 				Win32DeleteFile(string_args(path));
dll_import bool 				Win32FileExists(string_args(path)); 
dll_import memory_block Win32LoadFile( // Must call Win32FileExists() first
																			 string_args(path)); 
dll_import void 				Win32SaveFile( // Will create a new file if it doesn't exist. 
																			 // If it does it'll get replaced.
																			 void* data, ui32 dataSize, string_args(path)); 

// ******************** Directories ********************  //

dll_import void  Win32CreateDirectory(string_args(path)); 
dll_import void  Win32DeleteDirectory(string_args(path)); 
dll_import bool  Win32DirectoryExists(string_args(path)); 
dll_import char* // Needs to be freed 
								 Win32GetCurrentDirectory(); // Will return a full path but point to the last directory
dll_import char* // Needs to be freed
								 Win32GetCurrentDirectoryFullPath();
dll_import void  Win32SetCurrentDirectory(string_args(path)); 


#endif