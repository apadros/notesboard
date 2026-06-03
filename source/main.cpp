#include "apad_base_types.h"
#include "apad_error.h"
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_string.h"
#include "apad_time.h"
#include "apad_win32_gui.h"
#include "gui.h"

const ui16 NoteTextHeight = 15;
const f32  QuickClickTime = 0.2; // Seconds

struct note {
	rectangle background;
	char*     title;
	char*     text;
};

struct {
	// Temporary write box
	struct {
		ui16         left;
		ui16         bottom;
		memory_stack memory;
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
		note* 			 selectedByMouse;
		bool         moved; // To check whether to allow text writing
		memory_stack textMemory; // To push text being written onto. Once done, allocated onto specific note.
		note*        beingWritten;
	} notes;

	struct {
		ui16 				x;
		ui16 				y;
		ui16 				lastX;
		ui16 				lastY;
		bool 				leftDown;
		time_marker leftDownTime;
		bool        leftQuickClick;
	} mouse;
} state;

#define BeginNotesLoop(_varID) { \
	ForAll(state.notes.memory.size / sizeof(note)) { \
		auto* _varID = (note*)state.notes.memory.memory + it;
#define EndNotesLoop() } }

#define UnpackDimensions(_struct) (_struct).left, (_struct).bottom, (_struct).width, (_struct).height

void BeginTempWriting(ui16 left, ui16 bottom) {
	Assert(left != 0 && bottom != 0);
	Assert(IsValid(state.writeBox.memory) == false);
	state.writeBox.left = left;
	state.writeBox.bottom = bottom;
	// state.writeBox.background.height = GetSystemMetrics(SM_CYCURSOR); // Pixel height. @TODO - Does this take DPI into account?
	// Assert(state.writeBox.background.height != 0);
	state.writeBox.memory = AllocateStack();
}

bool TempTextHasBeenWritten() {
	return IsValid(state.writeBox.memory) == true && state.writeBox.memory.size > 0;
}

void AddTempWriting(char c) {
	if(TempTextHasBeenWritten() == true)
		state.writeBox.memory.size -= 1; // Remove \0
	char string[] = { c, '\0' }; // The '\0' won't be counted in PushString()
	PushString(string, true, state.writeBox.memory);
}

char* EndTempWriting() {
	Assert(IsValid(state.writeBox.memory) == true);

	char* ret = Null;
	if(state.writeBox.memory.size > 0) {
		PushString(Null, true, state.writeBox.memory);
		ret = AllocateString((char*)state.writeBox.memory.memory, Null);
	}

	FreeStack(state.writeBox.memory);
	state.writeBox.left = 0;
	state.writeBox.bottom = 0;

	return ret;
}

void EndNoteWriting() {
	Assert(state.notes.beingWritten != Null);
	state.notes.beingWritten->text = EndTempWriting();
	state.notes.beingWritten = Null;
}

bool TempTextIsBeingWritten() {
	return IsValid(state.writeBox.memory);
}

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
		state.mouse.leftQuickClick = false;
		if(osState.mouseMoved == true) {
			state.mouse.x = osState.mouseX;
			state.mouse.y = osState.mouseY;
		}
		if(osState.mouseLeftClickDown == true) {
			state.mouse.leftDown = true;
			state.mouse.x = osState.mouseX;
			state.mouse.y = osState.mouseY;
			state.mouse.leftDownTime = GetTimeMarker();
		}
		else if(osState.mouseLeftClickUp == true) {
			state.mouse.leftDown = false;
			state.mouse.x = osState.mouseX;
			state.mouse.y = osState.mouseY;

			Assert(state.mouse.leftDownTime > 0);
			if(GetTimeElapsedMilli(state.mouse.leftDownTime, GetTimeMarker()) / 1000 <= QuickClickTime)
				state.mouse.leftQuickClick = true;
		}

		// Selection of a note
		if(state.notes.selectedByMouse == Null && osState.mouseLeftClickDown == true && state.notes.memory.size > 0) {
			BeginNotesLoop(n) {
				if(Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(n->background)) == true) {
					state.notes.selectedByMouse = n;
					break;
				}
			}
			EndNotesLoop();
		}

		// Open a text box within a note or cancel text writing mode
		if(state.mouse.leftQuickClick == true) {
			note* n = Null;
			BeginNotesLoop(t) {
				if(Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(t->background)) == true) {
					n = t;
					break;
				}
			}
			EndNotesLoop();

			if(n != Null) {
				state.notes.beingWritten = n;
				BeginTempWriting(state.notes.beingWritten->background.left + NoteTextHeight, state.notes.beingWritten->background.bottom + NoteTextHeight);
				if(n->text != Null) {
					PushString(n->text, true, state.writeBox.memory);
					n->text = Null; // @TODO - Memory leak
				}
			}
		}
		else if(osState.mouseLeftClickDown == true && TempTextIsBeingWritten() == true)
			EndNoteWriting();

		// Toolbar notes button
		if(state.notes.selectedByMouse == Null && osState.mouseLeftClickDown == true && Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(state.toolbar.buttons[0].background)) == true) {
			// Create new note
			auto* n = (note*)Push(sizeof(note), state.notes.memory);
			n->background.left = 0;
			n->background.height = NoteTextHeight * 3;
			n->background.bottom = osState.mouseY - n->background.height / 2;
			n->background.width = 150;
			n->title = Null;
			n->text = Null;
			state.notes.selectedByMouse = n;
		}

		// Move a note
		if(state.notes.selectedByMouse != Null && state.mouse.leftDown == true && osState.mouseMoved == true) {
			si16 newLeft = state.notes.selectedByMouse->background.left + (state.mouse.x - state.mouse.lastX);
			Cap(newLeft, 0, canvas.width - state.notes.selectedByMouse->background.width);

			si16 newBottom = state.notes.selectedByMouse->background.bottom + (state.mouse.y - state.mouse.lastY);
			Cap(newBottom, 0, canvas.height - state.notes.selectedByMouse->background.height);

			state.notes.selectedByMouse->background.left = newLeft;
			state.notes.selectedByMouse->background.bottom = newBottom;

			state.notes.moved = true;
		}

		// Update text writing
		if(TempTextIsBeingWritten() == true) {
			auto* wb = &state.writeBox;
			if(osState.keyPressed != Null)
				AddTempWriting(osState.keyPressed);
			else if(osState.backspacePressed == true && TempTextHasBeenWritten() == true){
				if(wb->memory.size > 2) {
					((char*)wb->memory.memory)[wb->memory.size - 2] = '\0';
					wb->memory.size -= 1;
				}
				else
					wb->memory.size = 0;
			}
			else if(osState.escapePressed == true)
				EndNoteWriting();
			else if(osState.enterPressed == true) // Jump to next line
				AddTempWriting('\n');

			// @TODO - Click out to come out of text mode
		}

		// @TODO - When coming out of text writing, store string in relative note

		// Drop a note
		if(state.notes.selectedByMouse != Null && state.notes.moved == true && osState.mouseLeftClickUp == true)
			state.notes.selectedByMouse = Null;

		// @TODO - Check overlap with created notes

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
			glLineWidth(2);
			glColor3f(0, 0, 0);
			glBegin(GL_LINES);
			glVertex2s(tb->background.width, 0);
			glVertex2s(tb->background.width, tb->background.height);
			glEnd();
		}

		// Draw notes
		if(state.notes.memory.size > 0) {
			ui8 count = state.notes.memory.size / sizeof(note);
			ForAll(count) {
				auto* n = (note*)state.notes.memory.memory + it;
				DrawRectangle(UnpackDimensions(n->background), 255, 255, 255);
			}
		}

		// Draw border on a note selected with the mouse or that is being written to
		if(state.notes.selectedByMouse != Null || state.notes.beingWritten != Null) {
			auto* n = state.notes.selectedByMouse;
			if(n == Null)
				n = state.notes.beingWritten;
			Assert(n != Null);

			glBegin(GL_LINES);
			glColor3f(0, 0, 0);

			glVertex2f(n->background.left, n->background.bottom);
			glVertex2f(n->background.left, n->background.bottom + n->background.height);

			glVertex2f(n->background.left, n->background.bottom + n->background.height);
			glVertex2f(n->background.left + n->background.width, n->background.bottom + n->background.height);

			glVertex2f(n->background.left + n->background.width, n->background.bottom + n->background.height);
			glVertex2f(n->background.left + n->background.width, n->background.bottom);

			glVertex2f(n->background.left + n->background.width, n->background.bottom);
			glVertex2f(n->background.left, n->background.bottom);
			glEnd();
		}


		// @TODO - Is Win32GetMousePoswidthinClient() needed anymore?

		// Draw text within state.writeBox
		if(TempTextIsBeingWritten() == true) {
			ui16 cursorLeft = state.writeBox.left;
			if(TempTextHasBeenWritten() == true)
				cursorLeft = WriteText((const char*)state.writeBox.memory.memory, state.writeBox.left, state.writeBox.bottom, NoteTextHeight);

			// Cursor
			glLineWidth(2);
			glColor3f(0, 0, 0);
			glBegin(GL_LINES);
			glVertex2f(cursorLeft, state.writeBox.bottom);
			glVertex2f(cursorLeft, state.writeBox.bottom + NoteTextHeight);
			glEnd();
		}

		// Draw text within all notes
		BeginNotesLoop(n) {
			if(n->text != Null)
				WriteText(n->text, n->background.left + NoteTextHeight, n->background.bottom + NoteTextHeight, NoteTextHeight);

		}
		EndNotesLoop();

		Win32EndGUIUpdateLoop();
	}

	return 0;
}