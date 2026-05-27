#include "apad_win32_gui.h"

#include <windows.h>
#include <gl\gl.h>
#include "text.h"
GUIAppEntryPoint(instance) {
	Win32InitGUI("Bola Pad v0.0", instance);
	
	while(true) {
		Win32BeginGUIUpdateLoop();
		
		// glBegin(GL_TRIANGLES);
		// glColor3f(1, 0, 0);
		// glVertex2f(500, 600);
		// glVertex2f(500, 400);
		// glVertex2f(700, 500);
		// glEnd();
		
		WriteText("BOLA", 500, 500, 20);
		
		Win32EndGUIUpdateLoop();
	}
	
	return 0;
}