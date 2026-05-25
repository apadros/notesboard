#ifndef APAD_FILE_H
#define APAD_FILE_H

#include "apad_base_types.h"
#include "apad_intrinsics.h"
#include "apad_memory.h"
#include "apad_win32.h"

typedef memory_block file;

// A macro can be added here in case of porting to another OS

// ******************** Loading and saving ******************** //

#ifdef DeleteFile // Windows API
#undef DeleteFile
#endif

// For some reason can't declare these function pointers as dll_import
program_unique void 			(*DeleteFile)(const char* path) = Win32DeleteFile; // FileExists() must be called first
program_unique bool 			(*FileExists)(const char* path) = Win32FileExists;
program_unique file 			(*LoadFile)(const char* path) = Win32LoadFile; // FileExists() must be called first
dll_import     void 			  SaveFile(void* data, ui32 dataSize, const char* path); // Will create a new file if it doesn't exist; if it does it'll get replaced.
dll_import     void         SaveFile(file& f, const char* path);
program_unique void 			(*FreeFile)(file& f) = FreeMemory;
dll_import 		 bool   			IsValid(file f); // Defined in apad_memory.cpp
dll_import 		 const char*  GetFileNameAndExtension(const char* path); // Does not allocate a new string
dll_import 		 const char*  GetFileExtension(const char* path); // Does not allocate a new string

// ******************** Reading ******************** //

struct file_line {
	memory_stack data;
	ui8          count; // Number of individual data elements in the line, each separated by null terminators
};

#define              LineReadLoopHeader(_readIndexID, _file) for(ui32 _readIndexID = 0; _readIndexID < (_file).size; )
dll_import file_line ParseLine(file& f, ui32& readIndex); // Will treat any data between quotation marks as a single string
dll_import bool      LineIsValid(file_line& f);
dll_import char* 		 GetLineDataElement(file_line& line, ui8 index);
dll_import void 		 FreeLine(file_line& line); // Must be called after every call to ReadLine() once data is not needed

// ******************** Writing ******************** //

// Cause Windows
#ifdef CreateFile
#undef CreateFile
#endif

dll_import file CreateFile(); // Needs to be freed afterwards
dll_import void WriteToFile(void* data, ui32 size, file& f);
dll_import void WriteToFile(char* string, file& f);

#endif