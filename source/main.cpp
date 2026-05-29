#include "apad_base_types.h"
#include "apad_error.h"
#include "apad_maths.h"
#include "apad_win32_gui.h"

struct {
	struct {
		ui16 left;
		ui16 bottom;
		ui16 width;
		ui16 height;

		bool selected;

		ui16 selectionLeft;
		ui16 selectionBottom;
	} button;

	struct {
		ui16 leftClickX;
		ui16 leftClickY;
	} mouse;

} state;

f32 UI8ColourToF32(ui8 u) {
	return (f32)u / 255;
}

// @TODO - Export to APAD API
bool Overlap(ui16 x0, ui16 y0, ui16 left1, ui16 bottom1, ui16 width1, ui16 height1) {
	return x0 >= left1 && x0 <= left1 + width1 &&
				 y0 >= bottom1 && y0 <= bottom1 + height1;
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
	
	state.button.left = 500;
	state.button.bottom = 500;
	state.button.width = 200;
	state.button.height = 200;	

	while(true) {
		auto win32Events = Win32BeginGUIUpdateLoop();

		auto canvas = Win32GetProgramWindowClientSize();

		// Draw the background
		DrawRectangle(0, 0, canvas.width, canvas.height, 230, 230, 230);

		// Draw toolbar
		DrawRectangle(0, 0, 200, canvas.height, 255, 255, 255);

		// Draw button
		ui8 red = state.button.selected == true ? 255 : 0;

		if(win32Events.mouseLeftClickDown == true) {
			state.mouse.leftClickX = win32Events.mouseX;
			state.mouse.leftClickY = win32Events.mouseY;
		}

		// @TODO - Is Win32GetMousePosWithinClient() needed anymore?
		
		// auto pos = Win32GetMousePosWithinClient();
		if(win32Events.mouseLeftClickDown == true && Overlap(win32Events.mouseX, win32Events.mouseY, state.button.left, state.button.bottom, state.button.width, state.button.height) == true) {
			 state.button.selected = true;
			 state.button.selectionLeft = state.button.left;
			 state.button.selectionBottom = state.button.bottom;
		}
		else if(win32Events.mouseLeftClickUp == true && state.button.selected == true)
			state.button.selected = false;

		if(state.button.selected == true && win32Events.mouseMoved == true) {
			si16 dx = win32Events.mouseX - state.mouse.leftClickX;
			si16 dy = win32Events.mouseY - state.mouse.leftClickY;
			
			si16 newLeft = state.button.selectionLeft + dx;
			Cap(newLeft, 0, canvas.width - state.button.width);
			
			si16 newBottom = state.button.selectionBottom + dy;
			Cap(newBottom, 0, canvas.height - state.button.height);
			
			state.button.left = newLeft;
			state.button.bottom = newBottom;
		}
		
		DrawRectangle(state.button.left, state.button.bottom, state.button.width, state.button.height, red, 0, 0);

		//WriteText("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 500, 500, 20);

		Win32EndGUIUpdateLoop();
	}

	return 0;
}