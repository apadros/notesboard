#include "apad_base_types.h"
#include "apad_error.h"
#include "apad_win32_gui.h"

struct {
	struct {
		ui16 left;
		ui16 bottom;
		ui16 width;
		ui16 height;
	} button;
	
	
} state;

f32 UI8ColourToF32(ui8 u) {
	return (f32)u / 255;
}

#include <windows.h>
#include <gl\gl.h>
void DrawRectangle(ui16 left, ui16 bottom, ui16 width, ui16 height, ui8 r, ui8 g, ui8 b) {
	f32 rf = UI8ColourToF32(r);
	f32 gf = UI8ColourToF32(g);
	f32 bf = UI8ColourToF32(b);
	glBegin(GL_QUADS);
	glColor3f(rf, gf, bf);
	glVertex2f(left, bottom);
	glVertex2f(left + width, bottom);
	glVertex2f(left + width, bottom + height);
	glVertex2f(left, bottom + height);
	glEnd();
}

#include "text.h"
GUIAppEntryPoint(instance) {
	Win32InitGUI("Bola Pad v0.0", instance);
	
	while(true) {
		Win32BeginGUIUpdateLoop();
		
		auto canvas = Win32GetProgramWindowClientSize();
			
		
		// Draw the background
		DrawRectangle(0, 0, canvas.width, canvas.height, 230, 230, 230);
		
		// Draw toolbar
		DrawRectangle(0, 0, 200, canvas.height, 255, 255, 255);
		
		// Draw button
		state.button.left = 500;
		state.button.bottom = 500;
		state.button.width = 200;
		state.button.height = 200;
		ui8 red = 0;
		auto pos = Win32GetMousePosWithinClient();
		if(pos.x >= state.button.left && pos.x <= state.button.left + state.button.width &&
			 pos.y >= state.button.bottom && pos.y <= state.button.bottom + state.button.height)
			 red = 255;
		DrawRectangle(state.button.left, state.button.bottom, state.button.width, state.button.height, red, 0, 0);
		
		
		//WriteText("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 500, 500, 20);
		
		Win32EndGUIUpdateLoop();
	}
	
	return 0;
}