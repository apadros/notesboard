#ifndef APAD_WIN32_GUI_H
#define APAD_WIN32_GUI_H

#include <windows.h>
#include "apad_intrinsics.h"
#include "apad_maths.h"

// ******************** Core ******************** //

#define GUIAppEntryPoint(_instanceID) int CALLBACK WinMain(HINSTANCE _instanceID, HINSTANCE prevInstance, LPSTR commandLine, int commandShow)

dll_import void Win32InitGUI(const char* windowTitle /* Can be set to Null */, HINSTANCE instance);
	
struct win32_state {
	bool mouseLeftDown; // If false it is up
	bool mouseRightDown; // If false it is up
	bool mouseLeftDoubleClick; // Check this before mouseLeftDown
	
	bool   mouseMoved;
	vector mousePos;
	vector mouseTranslation; // Since last frame
	
	f32 mouseWheelRotation; // Where +/-1.0f represents a standard wheel notched rotation, positive for wheel rotating away from user.
													// Will return other values for freely-rotating mouse wheels
													// Value will update when mouse is within confines of program window, including title bar
	
	char keyPressed; // Will be Null if none
	bool backspacePressed;
	bool escapePressed;
	bool enterPressed;
	bool tabPressed;
	bool deletePressed;
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
	
	f32  lastFrameTime;
};
	
// These need to be encased in a while(true) loop
dll_import win32_state Win32BeginGUIUpdateLoop();
dll_import void   		 Win32EndGUIUpdateLoop(win32_state& state);

dll_import void DisplayLastWin32Error();

dll_import HWND Win32GetGUIWindowHandle();

// ******************** Others ******************** //

dll_import vector Win32GetProgramWindowClientSize();

dll_import vector Win32GetMousePosWithinClient(); // Return point will be capped to the dimensions of the client area

dll_import char* // Will be Null if cancelled or an error occurs
								 Win32OpenFileGUI(const char* directory, // Directory to open the GUI at, folders must separated by '\\'. Can be Null.
								 								  const char* filters);  // List of file types and extensions in format [type_string]\0[*.extension]\0...\0. E.g. "All\0*.*\0Text files\0*.txt\0\0"
																	
dll_import char* // Will be Null if cancelled or an error occurs
								 Win32SaveFileAsGUI(const char* directory, // Directory to open the GUI at, folders must separated by '\\'. Can be Null.
								 									  const char* filters);  // List of file types and extensions in format [type_string]\0[*.extension]\0...\0. E.g. "All\0*.*\0Text files\0*.txt\0\0"
																		
dll_import void Win32DisplayInfoBox(const char* string, bool error);

dll_import bool Win32MouseLeftDownThisFrame(win32_state& state);
dll_import bool Win32MouseLeftUpThisFrame(win32_state& state);


#endif
