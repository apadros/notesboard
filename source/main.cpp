#include <windows.h>
#include <gl\gl.h>
#include "apad_base_types.h"
#include "apad_error.h"
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_string.h"
#include "apad_time.h"
#include "apad_win32_gui.h"
#include "helpers.h"

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

		// Notes
		if(NoteIsBeingWritten() == false && state.mouse.leftQuickClick == true) { // Begin writing
			BeginNotesLoop(n) {
				if(Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(n->background)) == true) {
					BeginNoteWriting(n);
					break;
				}
			}
			EndNotesLoop();
		}
		else if(NoteIsBeingWritten() == true) { // Update  / end writing
			if(osState.mouseLeftClickDown == true || osState.escapePressed == true)
				EndNoteWriting();
			else {
				auto* n = GetNoteBeingWritten();
				if(osState.keyPressed != Null)
					AddNoteText(osState.keyPressed, n);
				else if(osState.backspacePressed == true && NoteHasText(n) == true){
					if(n->textMemory.size > 2) {
						((char*)n->textMemory.memory)[n->textMemory.size - 2] = '\0';
						n->textMemory.size -= 1;
					}
					else
						n->textMemory.size = 0;
				}
				else if(osState.enterPressed == true) // Jump to next line
					AddNoteText('\n', n);
			}
		}

		// Toolbar notes button
		if(state.notes.selectedByMouse == Null && osState.mouseLeftClickDown == true && Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(state.toolbar.buttons[0].background)) == true) {
			// Create new note
			auto* n = (note*)Push(sizeof(note), state.notes.memory);
			n->background.left = 0;
			n->background.height = NoteTextHeight * 3;
			n->background.bottom = osState.mouseY - n->background.height / 2;
			n->background.width = NoteMinWidth;
			n->title = Null;
			n->textMemory = AllocateStack();
			n->hasBulletPoints = false;
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
		if(NoteIsBeingWritten() == true) {
			auto* n = GetNoteBeingWritten();
			auto  textOrigin = GetNoteTextStart(n);
			ui16 	cursorLeft = textOrigin.x;
			ui16 	cursorBottom = textOrigin.y;
			if(NoteHasText(n) == true) {
				auto box = WriteText((const char*)n->textMemory.memory, textOrigin.x, textOrigin.y, NoteTextHeight);
				cursorLeft = box.cursorLeft;
				cursorBottom = box.edges.bottom;
				
				// Resize note
				n->background.width = Max(NoteMinWidth, box.edges.width + NoteTextBorder * 2);
				ui16 top = n->background.bottom + n->background.height;
				n->background.height = box.edges.height + NoteTextBorder * 2;
				n->background.bottom = top - n->background.height;
			}

			// Cursor
			glLineWidth(2);
			glColor3f(0, 0, 0);
			glBegin(GL_LINES);
			glVertex2f(cursorLeft, cursorBottom);
			glVertex2f(cursorLeft, cursorBottom + NoteTextHeight);
			glEnd();
		}

		// Draw text within all notes and update their size
		BeginNotesLoop(n) {
			if(NoteHasText(n) == true)
				WriteText((const char*)n->textMemory.memory, GetNoteTextStart(n).x, GetNoteTextStart(n).y, NoteTextHeight);
		}
		EndNotesLoop();

		Win32EndGUIUpdateLoop();
	}

	return 0;
}