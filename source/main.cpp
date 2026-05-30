#include "apad_base_types.h"
#include "apad_error.h"
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_string.h"
#include "apad_win32_gui.h"
#include "gui.h"

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
		ui16 left;
		ui16 bottom;
		ui16 width;
		ui16 height; // Set to mouse height + 20%
		char text[2] = { '\0', '\0'};
		bool writingText;
	} writeBox;
	
	struct {
		rectangle background;
		struct {
			rectangle 	background;
			const char* text;
		} buttons[3];
	} toolbar;

} state;

#define UnpackDimensions(_struct) (_struct).left, (_struct).bottom, (_struct).width, (_struct).height

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

GUIAppEntryPoint(instance) {
	Win32InitGUI("Bola Pad v0.0", instance);
	
	state.button.left = 500;
	state.button.bottom = 500;
	state.button.width = 200;
	state.button.height = 200;	
	
	auto textStack = AllocateStack();
	
	// Init toolbar
	{
		auto* tb = &state.toolbar;
		tb->background.left = 0;
		tb->background.bottom = 0;
		tb->background.width = 150;
		tb->background.height = Win32GetProgramWindowClientSize().height;
		
		// Init buttons, starting at the top
		ui16 spaceBetween = 50;
		ui16 size = (f32)tb->background.width * 2 / 3;
		ui16 textHeight = size / 4;
		ForAll(3) {
			tb->buttons[it].background.width = (f32)tb->background.width * 2 / 3;
			tb->buttons[it].background.left = tb->background.left + tb->background.width / 2 - tb->buttons[it].background.width / 2;
			tb->buttons[it].background.height = tb->buttons[it].background.width;
			tb->buttons[it].background.bottom = tb->background.height - (spaceBetween + tb->buttons[it].background.height + textHeight) * (it + 1);
		}
		tb->buttons[0].text = AllocateString("Button 1", Null);
		tb->buttons[1].text = AllocateString("Button 2", Null);
		tb->buttons[2].text = AllocateString("Button 3", Null);
	}
	while(true) {
		auto win32Events = Win32BeginGUIUpdateLoop();
		auto canvas = Win32GetProgramWindowClientSize();
		
		// Draw the background
		DrawRectangle(0, 0, canvas.width, canvas.height, 230, 230, 230);
		
		// Draw the toolbar
		{
			auto* tb = &state.toolbar;
			DrawRectangle(UnpackDimensions(state.toolbar.background), 255, 255, 255);
			
			// For now add 3 buttons, starting from the top
			ForAll(3) {
				DrawRectangle(UnpackDimensions(tb->buttons[it].background), 255, 0, 0);
				
			}
			
			// Draw separator
			glLineWidth(3);
			glColor3f(0, 0, 0);
			glBegin(GL_LINES);
			glVertex2s(tb->background.width, 0);
			glVertex2s(tb->background.width, tb->background.height);
			glEnd();
		}

		// Draw button
		ui8 red = state.button.selected == true ? 255 : 0;

		// @TODO - Is Win32GetMousePoswidthinClient() needed anymore?
		
		if(win32Events.mouseLeftClickDown == true) {
			if(textStack.size > 0)
				ResetStack(textStack);
			state.writeBox.height = GetSystemMetrics(SM_CYCURSOR); // Pixel height. @TODO - Does this take DPI into account?
			Assert(state.writeBox.height != 0);
			state.writeBox.left = win32Events.mouseX;
			state.writeBox.bottom = win32Events.mouseY - state.writeBox.height / 2;
			state.writeBox.width = 50; // @TODO - Placeholder, update
			state.writeBox.writingText = true;
		}
		
		if(state.writeBox.writingText == true && win32Events.keyPressed != Null) {
			if(textStack.size > 0)
				textStack.size -= sizeof(char); // Remove \0
			char text[] = { win32Events.keyPressed, '\0' };
			PushString(text, true, textStack);
		}
		
		if(textStack.size > 0)
			WriteText((const char*)textStack.memory, state.writeBox.left, state.writeBox.bottom, state.writeBox.height);
		
		// Code to select and drag a button
		#if 0
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
		#endif
		
		DrawRectangle(state.button.left, state.button.bottom, state.button.width, state.button.height, red, 0, 0);

		//WriteText("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 500, 500, 20);

		Win32EndGUIUpdateLoop();
	}

	return 0;
}