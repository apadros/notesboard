#include <windows.h>
#include <gl\gl.h>
#include "apad_base_types.h"
#include "apad_error.h"
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_opengl.h"
#include "apad_string.h"
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
		auto* tb = &state.toolBar;
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

	state.notes.memory = AllocateMemory(sizeof(note) * 10);

	while(true) {
		auto osState = Win32BeginGUIUpdateLoop();
		auto canvas = Win32GetProgramWindowClientSize();

		// Update mouse state
		state.mouse.translationX = 0;
		state.mouse.translationY = 0;
		if(osState.mouseX != 0 && osState.mouseY != 0) {
			state.mouse.translationX = osState.mouseX - state.mouse.x;
			state.mouse.translationY = osState.mouseY - state.mouse.y;
			state.mouse.x = osState.mouseX;
			state.mouse.y = osState.mouseY;
		}
		if(osState.mouseLeftClickDown == true)
			state.mouse.leftDown = true;
		else if(osState.mouseLeftClickUp == true)
			state.mouse.leftDown = false;
		if(osState.mouseRightClickDown == true)
			state.mouse.rightDown = true;
		else if(osState.mouseRightClickUp == true)
			state.mouse.rightDown = false;
		
		// Selection of the title bar
		if(osState.mouseLeftDoubleClick == true && Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(state.titleBar.background)) == true) {
			state.notes.selected = Null;
			auto* tb = &state.titleBar;
			BeginWriting(&tb->textMemory, &tb->background, TitleBarTextHeight);
		}
		
		// Toolbar
		if(osState.mouseLeftClickDown == true && Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(state.toolBar.buttons[0].background)) == true && state.notes.justCreated == false) { // Create new note
			note* n = Null;
			
			// Search for a free slot
			BeginNotesMemoryLoop(t) {
				if(NoteMemoryIsInUse(t) == false) {
					n = t;
					break;
				}
			}
			EndNotesMemoryLoop();
			
			// If not found, allocate more memory
			if(n == Null) {
				auto newBlock = AllocateMemory(state.notes.memory.size * 2);
				CopyMemory(state.notes.memory.memory, state.notes.memory.size, newBlock.memory);
				FreeMemory(state.notes.memory);
				state.notes.memory = newBlock;
				n = (note*)((ui8*)state.notes.memory.memory + state.notes.memory.size / 2);
			}
			
			Assert(n != Null);
			n->background.left = 0;
			n->background.height = NoteTextHeight * 3;
			n->background.bottom = osState.mouseY - n->background.height / 2;
			n->background.width = NoteMinWidth;
			n->title = Null;
			n->textMemory = AllocateStack();
			state.notes.selected = n;
			state.notes.justCreated = true;
		}
		else if(state.notes.selected != Null && osState.mouseLeftClickDown == true && Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(state.toolBar.buttons[1].background)) == true && TextIsBeingWritten() == true) { // Add a bullet point
			Assert(state.textUpdate.textMemory != Null);
			char* text = (char*)state.textUpdate.textMemory->memory;
			auto  length = GetStringLength(text);
			if(state.textUpdate.textMemory->size == 0 || state.textUpdate.textMemory->size > 1 && text[length - 1] != '\b')
				AddText('\b');
		}

		// Notes
		{
			if(osState.mouseLeftClickDown == true) { // Select and / or drag and deselection
				bool overlap = false;
				BeginNotesLoop(n) {
					if(NoteMemoryIsInUse(n) == true && Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(n->background)) == true) {
						state.notes.selected = n;
						state.notes.moving = true;
						if(TextIsBeingWritten() == true) // If text was already being written elsewhere
							EndWriting();
						overlap = true;
						break;
					}
				}
				EndNotesLoop();
				
				if(overlap == false)
					state.notes.selected = false;
			}
			
			// Move
			if(state.notes.moving == true) {
				Assert(state.notes.selected != Null);
				
				si16 newLeft = state.notes.selected->background.left + state.mouse.translationX;
				if(state.notes.justCreated == true) {
					Cap(newLeft, 0, canvas.width - state.notes.selected->background.width);
				}
				else {
					Cap(newLeft, state.toolBar.background.left + state.toolBar.background.width, canvas.width - state.notes.selected->background.width);
				}
	
				si16 newBottom = state.notes.selected->background.bottom + state.mouse.translationY;
				Cap(newBottom, 0, state.titleBar.background.bottom - state.notes.selected->background.height);
				
				// When moving a new note outside of the toolbar, ensure it can't be moved back in
				if(state.notes.justCreated == true && newLeft >= state.toolBar.background.left + state.toolBar.background.width)
					state.notes.justCreated = false;
	
				state.notes.selected->background.left = newLeft;
				state.notes.selected->background.bottom = newBottom;
			}
			
			// Stop moving and drop
			if(state.notes.moving == true && osState.mouseLeftClickUp == true) {
				state.notes.moving = false;
				
				// If the note was just created, drop it outside of the toolbar
				Assert(state.notes.selected != Null);
				if(state.notes.selected->background.left < state.toolBar.background.left + state.toolBar.background.width) {
					Assert(state.notes.justCreated == true);
					state.notes.selected->background.left = state.toolBar.background.left + state.toolBar.background.width;
				}
				
				state.notes.justCreated = false;
			}
			
			// Begin writing
			if(state.notes.selected != Null && osState.mouseLeftDoubleClick == true && Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(state.notes.selected->background)) == true)
				BeginWriting(&state.notes.selected->textMemory, &state.notes.selected->background, NoteTextHeight);
			
			// Delete note
			if(state.notes.selected != Null && TextIsBeingWritten() == false && (osState.deletePressed == true || osState.backspacePressed == true)) {
				FreeStack(state.notes.selected->textMemory);
				ClearMemory(state.notes.selected, sizeof(note));
				state.notes.selected = Null;
			}
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
			else if(osState.enterPressed == true) { // Jump to next line + exceptions
				if(tu->textMemory == &(state.titleBar.textMemory)) // If we're writing on the title bar
					EndWriting();
				else {
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
			}
			else if(tu->textMemory->size > 2 && osState.tabPressed == true) { // Remove bullet point if tab is pressed after it
				char* text = (char*)tu->textMemory->memory;
				auto  length = GetStringLength(text);
				if(text[length - 1] == '\b')
					text[length - 1] = ' ';
			}
			else if(osState.escapePressed == true || osState.mouseLeftClickDown && Overlap(state.mouse.x, state.mouse.y, UnpackDimensions(*tu->containerBackground)) == false) // End writing
				EndWriting();
		}
		
		// Reset the projection matrix
		{	
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			AssertOpenGL();
		
			auto size = Win32GetProgramWindowClientSize();
			Assert(size.width > 0 && size.height > 0);
			glOrtho(0, size.width, 0, size.height, -1, 1);
			AssertOpenGL();
		}
		
		// Draw the background
		DrawRectangle(0, 0, canvas.width, canvas.height, 230, 230, 230);
		
		// Draw the toolbar
		{
			auto* tb = &state.toolBar;
			DrawRectangle(UnpackDimensions(state.toolBar.background), 255, 255, 255); // Background

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
			AssertOpenGL();
		}
		
		// Draw title bar
		{
			auto* tb = &state.titleBar;
			DrawRectangle(UnpackDimensions(tb->background), 255, 255, 255);
			if(tb->textMemory.size > 0) {
				auto box = WriteText((char*)tb->textMemory.memory, tb->background.left + tb->background.width / 2, tb->background.bottom + tb->background.height / 2 - TitleBarTextHeight / 2, TitleBarTextHeight, true);
				if(state.textUpdate.textMemory == &tb->textMemory)
					SetCursorPos(box.cursorLeft, box.edges.bottom);
			}
			
			// Draw separator
			glLineWidth(3);
			glColor3f(0, 0, 0);
			glBegin(GL_LINES);
			glVertex2s(tb->background.left, tb->background.bottom);
			glVertex2s(tb->background.left + tb->background.width, tb->background.bottom);
			glEnd();
			AssertOpenGL();
		}
		
		// Update scaling
		if(osState.mouseWheelRotation != 0.0f) {
			// point mousePosPre = ConvertToProjectionSpace(state.mouse.x, state.mouse.y);
			state.projection.scale += osState.mouseWheelRotation / 10;
			if(state.projection.scale <= 0.0f)
				state.projection.scale = 0.1f;
			// point mousePosPost = ConvertToProjectionSpace(state.mouse.x, state.mouse.y);
			// state.projection.translationX += mousePosPost.x - mousePosPre.x;
			// state.projection.translationY += mousePosPost.y - mousePosPre.y;
			// state.projection.translationX += (state.mouse.x - state.mouse.x * percDelta) * state.projection.scale * percDelta / 2;
			// state.projection.translationY += (state.mouse.y - state.mouse.y * percDelta) * state.projection.scale * percDelta / 2;
		}
		if(state.projection.scale != 1.0f) {
			glMatrixMode(GL_PROJECTION);
			glScalef(state.projection.scale, state.projection.scale, 1.0f);
			AssertOpenGL();
		}
		
		// Update translations
		if(state.mouse.rightDown == true && (state.mouse.translationX != 0 || state.mouse.translationY != 0)) {
			state.projection.translationX += state.mouse.translationX;
			state.projection.translationY += state.mouse.translationY;
		}
		if(state.projection.translationX != 0 || state.projection.translationY != 0) {	
			glMatrixMode(GL_PROJECTION);
			glTranslatef(state.projection.translationX / state.projection.scale, state.projection.translationY / state.projection.scale, Null);
			AssertOpenGL();
		}
		
		// Draw notes
		BeginNotesMemoryLoop(n) {
			if(NoteMemoryIsInUse(n) == true)
				DrawRectangle(UnpackDimensions(n->background), 255, 255, 255);
		}
		EndNotesMemoryLoop();

		// Draw border on a selected note
		if(state.notes.selected != Null) {
			auto* n = state.notes.selected;
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
			AssertOpenGL();
		}

		// @TODO - Is Win32GetMousePoswidthinClient() needed anymore?

		// Draw notes text, update note background rectangle, update cursor
		BeginNotesLoop(n) {
			if(NoteMemoryIsInUse(n) == true) {
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
			AssertOpenGL();
		}

		Win32EndGUIUpdateLoop();
	}

	return 0;
}