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
		
		// Reset the projection matrix and draw the canvas background
		{	
			ResetProjectionMatrix();
			DrawRectangle(0, 0, canvas.width, canvas.height, 230, 230, 230);
		}

		// Update mouse state
		state.mouse.translation = { 0, 0 };
		if(osState.mouseX != 0 && osState.mouseY != 0) {
			state.mouse.translation.x = osState.mouseX - state.mouse.pos.x;
			state.mouse.translation.y = osState.mouseY - state.mouse.pos.y;
			state.mouse.pos.x = osState.mouseX;
			state.mouse.pos.y = osState.mouseY;
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
		if(osState.mouseLeftDoubleClick == true && Overlap(state.mouse.pos.x, state.mouse.pos.y, UnpackDimensions(state.titleBar.background)) == true) {
			if(TextIsBeingWritten() == true && TitleIsBeingUpdated() == false)
				EndWriting();
			state.notes.selected = Null;
			BeginWriting(&state.titleBar.textMemory, &state.titleBar.background, TitleBarTextHeight);
			goto label_rendering;
		}
		
		// Toolbar
		if(MouseLeftClickThisFrame() == true && Overlap(state.mouse.pos.x, state.mouse.pos.y, UnpackDimensions(state.toolBar.buttons[0].background)) == true) { // Create new note
			if(TextIsBeingWritten() == true)
				EndWriting();
			
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
			n->background.height = NoteTextHeight * 3;
			n->background.width = NoteMinWidth;
			auto pos = ConvertToCanvasSpace(0, osState.mouseY - n->background.height / 2);
			n->background.left = pos.x;
			n->background.bottom = pos.y;			
			n->title = Null;
			n->textMemory = AllocateStack();
			PushString(Null, true, n->textMemory);
			state.notes.selected = n;
			state.notes.justCreated = true;
			
			goto label_rendering;
		}
		else if(NoteIsBeingUpdated() == true && MouseLeftClickThisFrame() == true && Overlap(state.mouse.pos.x, state.mouse.pos.y, UnpackDimensions(state.toolBar.buttons[1].background)) == true) { // Add a bullet point
			Assert(state.textUpdate.textMemory != Null);
			char* text = (char*)state.textUpdate.textMemory->memory;
			auto  length = GetStringLength(text);
			Assert(state.textUpdate.cursorIndex <= length);
			if(state.textUpdate.cursorIndex == 0 || state.textUpdate.cursorIndex >= 1 && text[state.textUpdate.cursorIndex - 1] != '\b') // If the char at the cursor position is not a bullet point
				AddText('\b');
				
			goto label_rendering;
		}

		// Notes
		if(osState.mouseLeftDoubleClick == true) { // Begin writing regardles of whether a note is selected @TODO - Technically a note would have already been selected be 1st mouse click, simplify?
			auto* previouslySelected = state.notes.selected;
			state.notes.selected = Null;
		
			auto mousePosCanvas = ConvertToCanvasSpace(state.mouse.pos.x, state.mouse.pos.y);
			BeginNotesLoop(n) {
				if(NoteMemoryIsInUse(n) == true && Overlap(mousePosCanvas.x, mousePosCanvas.y, UnpackDimensions(n->background)) == true) {
					state.notes.selected = n;
					state.notes.moving = true;
					break;
				}
			}
			EndNotesLoop();
			
			// If we selected a different note or text was being written elsewhere, end writing
			if(TextIsBeingWritten() == true && (state.notes.selected == Null || previouslySelected != state.notes.selected))
				EndWriting();
			
			if(state.notes.selected != Null && previouslySelected == state.notes.selected)
				BeginWriting(&state.notes.selected->textMemory, &state.notes.selected->background, NoteTextHeight);
		}
		else if(MouseLeftClickThisFrame() == true) { // Select
			auto* previouslySelected = state.notes.selected;
			state.notes.selected = Null;
		
			auto mousePosCanvas = ConvertToCanvasSpace(state.mouse.pos.x, state.mouse.pos.y);
			BeginNotesLoop(n) {
				if(NoteMemoryIsInUse(n) == true && Overlap(mousePosCanvas.x, mousePosCanvas.y, UnpackDimensions(n->background)) == true) {
					state.notes.selected = n;
					state.notes.moving = true;
					break;
				}
			}
			EndNotesLoop();
			
			// If we selected a different note or text was being written elsewhere, end writing
			if(TextIsBeingWritten() == true && (state.notes.selected == Null || previouslySelected != state.notes.selected))
				EndWriting();
		}
		else if(state.notes.moving == true && osState.mouseLeftClickUp == false) { // Move
			Assert(state.notes.selected != Null);
			
			// Mouse moves in viewport space
			vector newPosCanvas = { state.notes.selected->background.left + (f32)state.mouse.translation.x / state.canvas.scale, 
														  state.notes.selected->background.bottom + (f32)state.mouse.translation.y / state.canvas.scale };
														 
			// When moving a new note outside of the toolbar, ensure it can't be moved back in
			if(state.notes.justCreated == true) {
				f32 toolbarEdgeCanvas = ConvertToCanvasSpace(state.toolBar.background.left + state.toolBar.background.width, Null).x;
				if(newPosCanvas.x >= toolbarEdgeCanvas)
					state.notes.justCreated = false;
			}
			
			state.notes.selected->background.left = newPosCanvas.x;
			state.notes.selected->background.bottom = newPosCanvas.y;
		}
		else if(state.notes.moving == true && osState.mouseLeftClickUp == true) { // Drop
			state.notes.moving = false;
			
			// If the note was just created, drop it outside of the toolbar
			Assert(state.notes.selected != Null);
			f32 toolbarEdgeCanvas = ConvertToCanvasSpace(state.toolBar.background.left + state.toolBar.background.width, Null).x;
			if(state.notes.selected->background.left < toolbarEdgeCanvas && state.notes.justCreated == true)
				state.notes.selected->background.left = toolbarEdgeCanvas;
			
			state.notes.justCreated = false;
		}
		else if(state.notes.selected != Null && TextIsBeingWritten() == false && (osState.deletePressed == true || osState.backspacePressed == true)) { // Delete note
			FreeStack(state.notes.selected->textMemory);
			ClearMemory(state.notes.selected, sizeof(note));
			state.notes.selected = Null;
		}
		
		// Update text somewhere
		if(TextIsBeingWritten() == true) {
			auto* tu = &state.textUpdate;
			
			if(osState.keyPressed != Null) // Push text
				AddText(osState.keyPressed);
			else if(osState.backspacePressed == true){
				Assert(tu->textMemory->size >= 1);
				if(tu->cursorIndex > 0) {
					Remove(sizeof(char), tu->cursorIndex - 1, *tu->textMemory);
					tu->cursorIndex -= 1;
				}
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
			else if(tu->cursorIndex >= 1 && ((char*)(tu->textMemory->memory))[tu->cursorIndex - 1] == '\b' && osState.tabPressed == true) // Remove bullet point if tab is pressed after it
				((char*)tu->textMemory->memory)[tu->cursorIndex - 1] = ' ';
			else if(osState.escapePressed == true) // Esc hit, end writing
				EndWriting();
			else if(osState.mouseLeftClickDown == true && MouseIsWithinToolbar() == false) { // Left mouse click outside of the tool bar, check where and decide
				vector mousePos = { state.mouse.pos.x, state.mouse.pos.y };
				if(tu->containerBackground != &state.titleBar.background) // If it's not the title bar, check for overlap in canvas space
					mousePos = ConvertToCanvasSpace(state.mouse.pos.x, state.mouse.pos.y);
				if(Overlap(mousePos.x, mousePos.y, UnpackDimensions(*tu->containerBackground)) == false)
					EndWriting();
			}
			else if(osState.leftPressed == true && tu->cursorIndex >= 1)
				tu->cursorIndex -= 1;
			else if(osState.rightPressed == true && tu->cursorIndex < (tu->textMemory->size - 1))
				tu->cursorIndex += 1;
			
			// Update cursor position
			if(TextIsBeingWritten() == true) { // In case EndWriting() was called above
				char* string = (char*)(tu->textMemory->memory);
				auto  length = tu->textMemory->size - 1;
				Assert(tu->cursorIndex <= length);
				f32   x = 0; 
				f32   y = 0;
				ForAll(tu->cursorIndex) {
					if(string[it] == '\n') {
						y -= tu->cursorHeight * 1.5f;
						x = 0;
					}
					else
						x += tu->cursorHeight * 1.5f;
				}
				x -= tu->cursorHeight * 0.25f; // Place half way between 2 glyphs
				
				if(tu->containerBackground == &state.titleBar.background) { // Writing on the title bar
					auto fullLength = length * tu->cursorHeight * 1.5f; // Including a small space after the last glyph
					x += state.titleBar.background.left + state.titleBar.background.width / 2 - fullLength / 2;
					y += state.titleBar.background.bottom + state.titleBar.background.height / 2 - TitleBarTextHeight / 2;
				}
				else { // Writing on a note
					x += tu->containerBackground->left + NoteTextBorder;
					y += tu->containerBackground->bottom + tu->containerBackground->height - NoteTextBorder - NoteTextHeight;
				}
				
				SetCursorPos(x, y);
			}
		}
		
		// Update scaling
		if(osState.mouseWheelRotation != 0.0f) {
			auto mousePosPre = ConvertToCanvasSpace(state.mouse.pos);
			state.canvas.scale += osState.mouseWheelRotation / 10;
			if(state.canvas.scale <= 0.0f)
				state.canvas.scale = 0.1f;
			auto   mousePosPost = ConvertToCanvasSpace(state.mouse.pos);
			vector mouseTranslationCanvas = mousePosPost - mousePosPre;
			state.canvas.translation += mouseTranslationCanvas * state.canvas.scale;
		}
		
		// Update translations
		if(state.mouse.rightDown == true && (state.mouse.translation.x != 0 || state.mouse.translation.y != 0)) {
			state.canvas.translation.x += state.mouse.translation.x;
			state.canvas.translation.y += state.mouse.translation.y;
		}
		
		// Use gotos to keep things tidy instead of if else everywhere
		label_rendering:
		
		SetCanvasProjetionMatrix();
		
		// Draw notes
		BeginNotesMemoryLoop(n) {
			if(NoteMemoryIsInUse(n) == true) {
				bool draw = true;
				if(state.notes.selected != Null && state.notes.selected == n && state.notes.justCreated == true) // Recently created notes will be drawn in front of the UI, further down
					draw = false;
					
				if(draw == true)
					DrawRectangle(UnpackDimensions(n->background), 255, 255, 255);
			}
		}
		EndNotesMemoryLoop();

		// Draw border on a selected note
		if(state.notes.selected != Null && state.notes.justCreated == false)
			DrawBorder(state.notes.selected->background);

		// @TODO - Is Win32GetMousePoswidthinClient() needed anymore?

		// Draw notes text, update note background rectangle, update cursor
		BeginNotesLoop(n) {
			if(NoteMemoryIsInUse(n) == true) {
				rectangle textBox = {};
				vector    textOrigin = { n->background.left + NoteTextBorder, n->background.bottom + n->background.height - NoteTextBorder - NoteTextHeight };
				if(n->textMemory.size > 1) // Note has text
					textBox = WriteText((const char*)n->textMemory.memory, textOrigin.x, textOrigin.y, NoteTextHeight, false);
				
				// Resize note
				if(TextIsBeingWritten() == true && state.textUpdate.containerBackground == &n->background) {
					n->background.width = GetMax(NoteMinWidth, textBox.width + NoteTextBorder * 2);
					f32 top = n->background.bottom + n->background.height;
					n->background.height = GetMax(NoteMinHeight, textBox.height + NoteTextBorder * 2);
					n->background.bottom = top - n->background.height;
				}
			}
		}
		EndNotesLoop();
		
		// Draw the overlying UI
		ResetProjectionMatrix();
			
		// Toolbar
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
		
		// Title bar
		{
			auto* tb = &state.titleBar;
			DrawRectangle(UnpackDimensions(tb->background), 255, 255, 255);
			if(tb->textMemory.size > 1) {
				WriteText((char*)tb->textMemory.memory, tb->background.left + tb->background.width / 2, tb->background.bottom + tb->background.height / 2 - TitleBarTextHeight / 2, TitleBarTextHeight, true);
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
			
		// If a note was just created, draw in front of the tool bar
		if(state.notes.selected != Null && state.notes.justCreated == true) {
			SetCanvasProjetionMatrix();
			DrawRectangle(UnpackDimensions(state.notes.selected->background), 255, 255, 255);
			DrawBorder(state.notes.selected->background);
		}
		
		// Draw cursor if needed
		if(state.textUpdate.textMemory != Null) {
			ResetProjectionMatrix();
			f32 cursorX = state.textUpdate.cursorPos.x;
			f32 cursorY = state.textUpdate.cursorPos.y;
			f32 cursorHeight = state.textUpdate.cursorHeight;
			if(state.textUpdate.containerBackground != &state.titleBar.background)
				SetCanvasProjetionMatrix();
			
			glLineWidth(2);
			glColor3f(0, 0, 0);
			glBegin(GL_LINES);
			glVertex2f(cursorX, cursorY);
			glVertex2f(cursorX, cursorY + cursorHeight);
			glEnd();
			AssertOpenGL();
		}

		Win32EndGUIUpdateLoop();
	}
	
	// Store state before next frame
	state.mouse.lastLeftDown = state.mouse.leftDown;

	return 0;
}