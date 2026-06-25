#ifndef APAD_WIN32_H
#define APAD_WIN32_H

#include "apad_base_types.h"
#include "apad_intrinsics.h"
#include "apad_memory.h"

#define ConsoleAppEntryPoint(_argumentsID, _argumentCountID) /* The first argument corresponds to the program name */ \
				  int main(int _argumentCountID, char** _argumentsID)

dll_import void Win32OutputDebugString(const char* string);
dll_import void Win32PrintStackBackTrace(); // Need to compile without optimizations and generate debug info for this to be useful

// ******************** Memory ********************  //

								 // Will automatically clear allocated memory
dll_import void* Win32AllocateMemory(ui32 size);
dll_import void  Win32FreeMemory(void* mem);

// ******************** Files ********************  //

dll_import void 				Win32DeleteFile(const char* path);
dll_import bool 				Win32FileExists(const char* path);
												// Calls Win32FileExists() first, returns if false
dll_import memory_block Win32LoadFile(const char* path);
												// Will create a new file if it doesn't exist. 
												// If it does it'll get replaced.
dll_import void 				Win32SaveFile(void* data, ui32 dataSize, const char* path);

// ******************** Directories ********************  //

dll_import void  Win32CreateDirectory(const char* path);
dll_import void  Win32DeleteDirectory(const char* path);
dll_import bool  Win32DirectoryExists(const char* path);
dll_import char* Win32GetCurrentDirectory(); // Will return a full path but point to the last directory
dll_import char* Win32GetCurrentDirectoryFullPath();
dll_import void  Win32SetCurrentDirectory(const char* path);


#endif