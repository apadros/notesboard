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
	
	// Init title bar
	{
		auto* tb = &state.titleBar;
		tb->background.width = Win32GetProgramWindowClientSize().width;
		tb->background.height = TitleBarHeight;
		tb->background.bottom = Win32GetProgramWindowClientSize().height - tb->background.height;
		tb->textMemory = AllocateStack();
		PushString("Title", true, tb->textMemory);
	}

	// Init toolbar
	{
		auto* tb = &state.toolbar;
		tb->background.left = 0;
		tb->background.bottom = 0;
		tb->background.width = ToolbarWidth;
		tb->background.height = Win32GetProgramWindowClientSize().height - state.titleBar.background.height;

		// Init buttons, starting at the top
		tb->textHeight = ToolbarTextHeight;
		ForAll(3) {
			auto* b = tb->buttons + it;
			b->background.width = ToobalIconWidth;
			b->background.left = tb->background.left + tb->background.width / 2 - b->background.width / 2;
			b->background.height = ToobalIconWidth;
			b->background.bottom = tb->background.height - (ToolVerticalSpaceBetweenIcons + b->background.height + tb->textHeight * 2) * (it + 1);
			b->textBottom = b->background.bottom - ToolbarTextHeight * 2;
		}
		tb->buttons[0].text = AllocateString("Note", Null);
		tb->buttons[1].text = AllocateString("Bullet point", Null);
		tb->buttons[2].text = AllocateString("Button", Null);
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
		
		// Selection of the title bar
		if(osState.mouseLeftClickDown == true && Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(state.titleBar.background)) == true) {
			auto* tb = &state.titleBar;
			state.textUpdate.textMemory = &(tb->textMemory);
			state.textUpdate.cursorHeight = TitleBarTextHeight;
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
		
		// Update text somewhere
		if(TextIsBeingWritten() == true) {
			auto* tu = &state.textUpdate;
			
			if(osState.keyPressed != Null) // Push text
				AddText(osState.keyPressed);
			else if(osState.backspacePressed == true && tu->textMemory->size > 1){
				if(tu->textMemory->size > 2) {
					((char*)tu->textMemory->memory)[tu->textMemory->size - 2] = '\0';
					tu->textMemory->size -= 1;
				}
				else
					tu->textMemory->size = 0;
			}
			else if(osState.enterPressed == true) { // Jump to next line
				// Scan back to see if the current line contains a bullet point
				bool  bulletPoint = false;
				char* text = (char*)tu->textMemory->memory;
				auto  length = GetStringLength(text);
				FromTo(length, 0) {
					char c = text[it];
					if(c == '\b') {
						bulletPoint = true;
						break;
					}
					else if(c == '\n')
						break;
				}
				
				AddText('\n');
				
				if(bulletPoint == true)
					AddText('\b');
			}
			else if(tu->textMemory->size > 2 && osState.tabPressed == true) { // Remove bullet point if tab is pressed after it
				char* text = (char*)tu->textMemory->memory;
				auto  length = GetStringLength(text);
				if(text[length - 1] == '\b')
					text[length - 1] = ' ';
			}
			else if(osState.escapePressed == true) // End writing
				tu->textMemory = Null;
		}

		// Notes
		if(TextIsBeingWritten() == false && state.mouse.leftQuickClick == true) { // Begin writing
			BeginNotesLoop(n) {
				if(Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(n->background)) == true) {
					BeginWriting(&(n->textMemory), NoteTextHeight);
					break;
				}
			}
			EndNotesLoop();
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
			state.notes.selectedByMouse = n;
		}
		else if(state.notes.selectedByMouse == Null && osState.mouseLeftClickDown == true && Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(state.toolbar.buttons[1].background)) == true && TextIsBeingWritten() == true) {
			Assert(state.textUpdate.textMemory != Null);
			char* text = (char*)state.textUpdate.textMemory->memory;
			auto  length = GetStringLength(text);
			if(state.textUpdate.textMemory->size == 0 || state.textUpdate.textMemory->size > 1 && text[length - 1] != '\b')
				AddText('\b');
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
				WriteText(b->text, GetMiddle(b->background).x, b->textBottom, tb->textHeight, true);
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
		if(state.notes.selectedByMouse != Null || TextIsBeingWritten() == true) {
			note* n = Null;
			if(state.notes.selectedByMouse != Null)
				n = state.notes.selectedByMouse;
			else {
				BeginNotesLoop(t) {
					if(state.textUpdate.textMemory == &(t->textMemory)) {
						n = t;
						break;
					}
				}
				EndNotesLoop();
			}
			
			if(n != Null) {
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
		}
		
		// Draw title bar
		{
			auto* tb = &state.titleBar;
			DrawRectangle(UnpackDimensions(tb->background), 255, 255, 255);
			if(tb->textMemory.size > 0) {
				auto box = WriteText((char*)tb->textMemory.memory, tb->background.left + tb->background.width / 2, tb->background.bottom + tb->background.height / 2 - TitleBarTextHeight / 2, TitleBarTextHeight, true);
				if(state.textUpdate.textMemory == &(tb->textMemory))
					SetCursorPos(box.cursorLeft, box.edges.bottom);
			}
			
			// Draw separator
			glLineWidth(3);
			glColor3f(0, 0, 0);
			glBegin(GL_LINES);
			glVertex2s(tb->background.left, tb->background.bottom);
			glVertex2s(tb->background.left + tb->background.width, tb->background.bottom);
			glEnd();
		}

		// @TODO - Is Win32GetMousePoswidthinClient() needed anymore?

		// Draw notes text, update note background rectangle, update cursor
		BeginNotesLoop(n) {
			text_box textBox = {};
			point    textOrigin = { n->background.left + NoteTextBorder, n->background.bottom + n->background.height - NoteTextBorder - NoteTextHeight };
			point    cursorPos = textOrigin;
			if(n->textMemory.size > 0) { // Note has text
				ui16 cursorLeft = textOrigin.x;
				ui16 cursorBottom = textOrigin.y;
				textBox = WriteText((const char*)n->textMemory.memory, textOrigin.x, textOrigin.y, NoteTextHeight, false);
				cursorPos.x = textBox.cursorLeft;
				cursorPos.y = textBox.edges.bottom;
			}
			
			// Update cursor
			if(TextIsBeingWritten() == true && &(n->textMemory) == state.textUpdate.textMemory)
				SetCursorPos(cursorPos.x, cursorPos.y);
			
			// Resize note
			n->background.width = GetMax(NoteMinWidth, textBox.edges.width + NoteTextBorder * 2);
			ui16 top = n->background.bottom + n->background.height;
			n->background.height = GetMax(NoteMinHeight, textBox.edges.height + NoteTextBorder * 2);
			n->background.bottom = top - n->background.height;
			
		}
		EndNotesLoop();
		
		// Draw cursor if needed
		if(state.textUpdate.textMemory != Null) {
			glLineWidth(2);
			glColor3f(0, 0, 0);
			glBegin(GL_LINES);
			glVertex2f(state.textUpdate.cursorX, state.textUpdate.cursorY);
			glVertex2f(state.textUpdate.cursorX, state.textUpdate.cursorY + state.textUpdate.cursorHeight);
			glEnd();
		}

		Win32EndGUIUpdateLoop();
	}

	return 0;
}