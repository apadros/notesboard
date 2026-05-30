#include "apad_base_types.h"
#include "apad_error.h"
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_string.h"
#include "apad_win32_gui.h"
#include "gui.h"

const ui16 NoteTextHeight = 25;
struct note {
	rectangle background;
	char*     title;
	char*     text;
};

struct {	
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
			ui16        textBottom;
		} 						buttons[3];
		ui16          textHeight;
	} toolbar;
	
	struct {
		memory_stack memory;
		note* 			 selected;
	} notes;
	
	struct {
		ui16 x;
		ui16 y;
		ui16 lastX;
		ui16 lastY;
		bool leftDown;
	} mouse;
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
	
	auto textStack = AllocateStack();
	
	// Init toolbar
	{
		auto* tb = &state.toolbar;
		tb->background.left = 0;
		tb->background.bottom = 0;
		tb->background.width = 150;
		tb->background.height = Win32GetProgramWindowClientSize().height;
		
		// Init buttons, starting at the top
		ui16 size = (f32)tb->background.width * 2 / 3;
		ui16 spaceBetween = (tb->background.width - size) / 2;
		tb->textHeight = size / 8;
		ForAll(3) {
			auto* b = tb->buttons + it;
			b->background.width = (f32)tb->background.width * 2 / 3;
			b->background.left = tb->background.left + tb->background.width / 2 - b->background.width / 2;
			b->background.height = b->background.width;
			b->background.bottom = tb->background.height - (spaceBetween + b->background.height + tb->textHeight) * (it + 1);
			b->textBottom = b->background.bottom - spaceBetween / 2;
		}
		tb->buttons[0].text = AllocateString("Button 1", Null);
		tb->buttons[1].text = AllocateString("Button 2", Null);
		tb->buttons[2].text = AllocateString("Button 3", Null);
	}
	
	state.notes.memory = AllocateStack();
	
	while(true) {
		auto osState = Win32BeginGUIUpdateLoop();
		auto canvas = Win32GetProgramWindowClientSize();
		
		// Update mouse state
		state.mouse.lastX = state.mouse.x;
		state.mouse.lastY = state.mouse.y;
		if(osState.mouseMoved == true) {
			state.mouse.x = osState.mouseX;
			state.mouse.y = osState.mouseY;
		}
		if(osState.mouseLeftClickDown == true) {
			state.mouse.leftDown = true;
			state.mouse.x = osState.mouseX;
			state.mouse.y = osState.mouseY;
		}
		else if(osState.mouseLeftClickUp == true) {
			state.mouse.leftDown = false;
			state.mouse.x = osState.mouseX;
			state.mouse.y = osState.mouseY;
		}
		
		// Check for left clicking on the first toolbar button
		if(state.notes.selected == Null && osState.mouseLeftClickDown == true && Overlap(osState.mouseX, osState.mouseY, UnpackDimensions(state.toolbar.buttons[0].background)) == true) {
			// Create new note
			auto* n = (note*)Push(sizeof(note), state.notes.memory);
			n->background.left = 0;
			n->background.height = 50;
			n->background.bottom = osState.mouseY - n->background.height / 2;
			n->background.width = 100;
			n->title = Null;
			n->text = Null;
			state.notes.selected = n;
		}
		
		// @TODO - Check overlap with created notes
		
		// A note is currently selected
		if(state.notes.selected != Null) { 
			 if(osState.mouseLeftClickUp == true) // Drop
			  state.notes.selected = Null;
			 else if(osState.mouseMoved == true) { // Move
				si16 newLeft = state.notes.selected->background.left + (state.mouse.x - state.mouse.lastX);
				Cap(newLeft, 0, canvas.width - state.notes.selected->background.width);
				
				si16 newBottom = state.notes.selected->background.bottom + (state.mouse.y - state.mouse.lastY);
				Cap(newBottom, 0, canvas.height - state.notes.selected->background.height);
				
				state.notes.selected->background.left = newLeft;
				state.notes.selected->background.bottom = newBottom;
			 }
		}
		
		// Draw the background
		DrawRectangle(0, 0, canvas.width, canvas.height, 230, 230, 230);
		
		// Draw the toolbar
		{
			auto* tb = &state.toolbar;
			DrawRectangle(UnpackDimensions(state.toolbar.background), 255, 255, 255); // Background
			
			ForAll(3) { // Buttons
				auto* b = tb->buttons + it;
				DrawRectangle(UnpackDimensions(b->background), 255, 0, 0);
				WriteText(b->text, b->background.left, b->textBottom, tb->textHeight);
			}
			
			// Draw separator
			glLineWidth(3);
			glColor3f(0, 0, 0);
			glBegin(GL_LINES);
			glVertex2s(tb->background.width, 0);
			glVertex2s(tb->background.width, tb->background.height);
			glEnd();
		}
		
		// Draw notes
		if(state.notes.memory.size > 0) {
			ui8 count = state.notes.memory.size % sizeof(note);
			ForAll(count) {
				auto* n = (note*)state.notes.memory.memory + it;
				DrawRectangle(UnpackDimensions(n->background), 0, 255, 0);
			}
		}

		// @TODO - Is Win32GetMousePoswidthinClient() needed anymore?
		
		#if 0
		if(osState.mouseLeftClickDown == true) {
			if(textStack.size > 0)
				ResetStack(textStack);
			state.writeBox.height = GetSystemMetrics(SM_CYCURSOR); // Pixel height. @TODO - Does this take DPI into account?
			Assert(state.writeBox.height != 0);
			state.writeBox.left = osState.mouseX;
			state.writeBox.bottom = osState.mouseY - state.writeBox.height / 2;
			state.writeBox.width = 50; // @TODO - Placeholder, update
			state.writeBox.writingText = true;
		}
		
		if(state.writeBox.writingText == true && osState.keyPressed != Null) {
			if(textStack.size > 0)
				textStack.size -= sizeof(char); // Remove \0
			char text[] = { osState.keyPressed, '\0' };
			PushString(text, true, textStack);
		}
		#endif
		
		if(textStack.size > 0)
			WriteText((const char*)textStack.memory, state.writeBox.left, state.writeBox.bottom, state.writeBox.height);
		
		//WriteText("ABCDEFGHIJKLMNOPQRSTUVWXYZ", 500, 500, 20);

		Win32EndGUIUpdateLoop();
	}

	return 0;
}