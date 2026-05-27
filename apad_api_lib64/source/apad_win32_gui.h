#ifndef APAD_WIN32_GUI_H
#define APAD_WIN32_GUI_H

#include <windows.h>
#include "apad_intrinsics.h"

#define GUIAppEntryPoint(_instanceID) int CALLBACK WinMain(HINSTANCE _instanceID, HINSTANCE prevInstance, LPSTR commandLine, int commandShow)

dll_import void Win32InitGUI(const char* windowTitle /* Can be set to Null */, HINSTANCE instance);
															 
// These need to be encased in a while(true) loop
dll_import void Win32BeginGUIUpdateLoop();
dll_import void Win32EndGUIUpdateLoop();

dll_import void DisplayLastWin32Error();

#endif
