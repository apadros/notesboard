#ifndef APAD_FILE_H
#define APAD_FILE_H

#include "apad_base_types.h"
#include "apad_intrinsics.h"
#include "apad_memory.h"
#include "apad_string_args.h"
#include "apad_win32.h"
#include "apad_win32_gui.h"

// Do not access the contents directly as these will change depending on whether
// the file was openend in read or write mode
typedef memory_stack file;

// ******************** Loading and saving ******************** //

#ifdef DeleteFile // Windows API
#undef DeleteFile
#endif

// For some reason can't declare the function pointers as dll_import

// File properties & info
dll_import 		 void* GetMemory(file f); // Calls into apad_memory API
dll_import 		 ui32  GetSize(file& f);
dll_import 		 bool  IsValid(file& f);
dll_import 		 char* GetFileNameAndExtension(string_args(path)); // Does not allocate a new string
dll_import 		 char* GetFileExtension(string_args(path)); // Does not allocate a new string

// I/O
program_unique void  (*DeleteFile)(string_args(path)) = Win32DeleteFile; // FileExists(path) must be called first
program_unique bool  (*FileExists)(string_args(path)) = Win32FileExists;
program_unique void  (*FreeFile)(file& f) = Free;
dll_export     file    LoadFile(string_args(path)); // FileExists(path) must be called first
program_unique char* (*OpenFileGUI)(string_args(directory), // Directory to open the GUI at, folders must separated by '\\'. Can be Null.
																		 string_args(filters))   // List of file types and extensions in format [type_string]\0[*.extension]\0...\0. E.g. "All\0*.*\0Text files\0*.txt\0\0" 
																		 = Win32OpenFileGUI; 
program_unique char*  (*SaveFileAsGUI)(string_args(directory), // Directory to open the GUI at, folders must separated by '\\'. Can be Null.
																			 string_args(filters))   // List of file types and extensions in format [type_string]\0[*.extension]\0...\0. E.g. "All\0*.*\0Text files\0*.txt\0\0" 
																			 = Win32SaveFileAsGUI; 
dll_import     void 	  SaveFile(void* data, ui32 dataSize, string_args(path)); // Will create a new file if it doesn't exist; if it does it'll get replaced.
dll_import     void     SaveFile(file& f, string_args(path));

// ******************** Reading ******************** //

struct file_line {
	memory_stack data;
	ui8          count; // Number of individual data elements in the line, each separated by null terminators
};

dll_import file_line ParseLine(file& f, ui32& readIndex); // Will treat any data between quotation marks as a single string
dll_import bool      IsValid(file_line& f);
dll_import char* 		 GetDataElement(file_line& line, ui8 index /* 0 based */);
dll_import void 		 Free(file_line& line); // Must be called after every call to ReadLine() once data is not needed

// ******************** Writing ******************** //

#ifdef CreateFile // Windows API
#undef CreateFile
#endif

dll_import file CreateFile(); // Needs to be freed afterwards
dll_import void Free(file&);
dll_import void WriteToFile(void* data, ui32 size, file& f);
dll_import void WriteToFile(string_args(string), file& f);

#endif