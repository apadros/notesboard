#ifndef APAD_WIN32_GUI_H
#define APAD_WIN32_GUI_H

#include <windows.h>
#include "apad_intrinsics.h"

struct rectangle {
	f32 left;
	f32 bottom;
	f32 width;
	f32 height;
};

struct point {
	union {
		ui16 x;
		ui16 width;
	};
	
	union {
		ui16 y;
		ui16 height;
	};
};
typedef point size;

// ******************** Core ******************** //

#define GUIAppEntryPoint(_instanceID) int CALLBACK WinMain(HINSTANCE _instanceID, HINSTANCE prevInstance, LPSTR commandLine, int commandShow)

dll_import void Win32InitGUI(const char* windowTitle /* Can be set to Null */, HINSTANCE instance);
	
struct win32_state {
	bool mouseLeftClickDown;
	bool mouseLeftClickUp;
	
	bool mouseRightClickDown;
	bool mouseRightClickUp;
	
	bool mouseMoved;
	ui16 mouseX; // Wlll only be updated during mouse move and click events
	ui16 mouseY; // Wlll only be updated during mouse move and click events
	
	char keyPressed; // Will be Null if none
	bool backspacePressed;
	bool escapePressed;
	bool enterPressed;
	bool tabPressed;
	bool leftPressed;
	bool rightPressed;
	bool downPressed;
	bool upPressed;
	
	bool capsLock;
	bool leftShift;
	bool rightShift;
	bool leftAlt;
	bool rightAlt;
	bool leftCtrl;
	bool rightCtrl;
};
	
// These need to be encased in a while(true) loop
dll_import win32_state Win32BeginGUIUpdateLoop();
dll_import void   		 Win32EndGUIUpdateLoop();

dll_import void DisplayLastWin32Error();

// ******************** Others ******************** //

dll_export size Win32GetProgramWindowClientSize();

dll_export point Win32GetMousePosWithinClient(); // Return point will be capped to the dimensions of the client area

#endif
