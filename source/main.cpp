#include <windows.h>
#include <gl\gl.h>
#include "apad_array.h"
#include "apad_base_types.h"
#include "apad_error.h"
#include "apad_file.h"
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_opengl.h"
#include "apad_string.h"
#include "apad_win32_gui.h"
#include "helpers.h"

#include <math.h>

GUIAppEntryPoint(instance) {
	Win32InitGUI("Bola Pad v0.0", instance);
	
	// Init top menu
	{
		auto* m = &state.topMenu;
		m->background.left = 0;
		m->background.width = Win32GetProgramWindowClientSize().x;
		m->background.height = TopMenuHeight;
		m->background.bottom = Win32GetProgramWindowClientSize().y - m->background.height;
		ForAll(GetArrayLength(m->buttons))
			m->buttons[it].left = TopMenuButtonWidth * it;
		m->buttons[0].text = AllocateString("Save");
		m->buttons[1].text = AllocateString("Load");
		m->buttons[2].text = AllocateString("Back");
	}
	
	// Init title bar
	{
		auto* tb = &state.titleBar;
		tb->background.width = Win32GetProgramWindowClientSize().width;
		tb->background.height = TitleBarHeight;
		tb->background.bottom = state.topMenu.background.bottom - tb->background.height;
		tb->text = AllocateTextBody(false);
		InsertText("Title", GetStringLength("Title"), tb->text, 0);
	}

	// Init toolbar
	{
		auto* tb = &state.toolBar;
		tb->background.left = 0;
		tb->background.bottom = 0;
		tb->background.width = ToolbarWidth;
		tb->background.height = state.topMenu.background.bottom - state.titleBar.background.height;

		// Init buttons, starting at the top
		tb->textHeight = ToolbarTextHeight;
		ForAll(GetArrayLength(tb->buttons)) {
			auto* b = tb->buttons + it;
			b->background.width = ToobalIconWidth;
			b->background.left = tb->background.left + tb->background.width / 2 - b->background.width / 2;
			b->background.height = ToobalIconWidth;
			b->background.bottom = tb->background.height - (ToolVerticalSpaceBetweenIcons + b->background.height + tb->textHeight * 2) * (it + 1);
			b->textBottom = b->background.bottom - ToolbarTextHeight * 2;
		}
		tb->buttons[0].text = AllocateString("Note");
		tb->buttons[1].text = AllocateString("Bullet point");
		tb->buttons[2].text = AllocateString("Note Title");
		tb->buttons[3].text = AllocateString("Colour Wheel");
	}

	state.notes.memory = AllocateMemory(sizeof(note) * 10);

	while(true) {
		auto osState = Win32BeginGUIUpdateLoop();
		auto canvas = Win32GetProgramWindowClientSize();
		
		// Reset the projection matrix and draw the canvas background
		{	
			SetGUIProjectionMatrix();
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
		
		// Top menu
		if(MouseOverlapsGUI(state.topMenu.background) == true) {
			auto* m = &state.topMenu;
			if(MouseLeftDownThisFrame() == true) {
				ForAll(GetArrayLength(m->buttons)) {
					if(state.mouse.pos.x >= m->buttons[it].left && state.mouse.pos.x < m->buttons[it].left + TopMenuButtonWidth) {
						if(it == 0) { // Save
							// Set correct directory
							if(StringsAreEqual(Win32GetCurrentDirectory(), "Boards") == false) {
								if(Win32DirectoryExists("Boards") == false)
									Win32CreateDirectory("Boards");
								Win32SetCurrentDirectory("Boards");
							}
							
							char* path = SaveFileAsGUI(Null, "Bola boards\0*.bb\0\0"); // Get save file path
							if(path != Null) {
								auto memory = AllocateStack();
								
								// Store board title
								if(GetTextLength(state.titleBar.text) > 0)
									PushString(GetTextStart(state.titleBar.text), false, memory);
								PushString(Null, true, memory);
								
								// Notes
								BeginNotesLoop(n) {
									if(NoteMemoryIsInUse(n) == true) {
										// Store the rectangle
										f32* f = PushType(f32, memory);
										*f = n->background.left;
										f = PushType(f32, memory);
										*f = n->background.bottom;
										f = PushType(f32, memory);
										*f = n->background.width;
										f = PushType(f32, memory);
										*f = n->background.height;
										
										// Text contents
										if(NoteHasTitle(n) == true)
											PushString(GetTextStart(n->title), false, memory);
										PushString(Null, true, memory);
										
										if(GetTextLength(n->text) > 0)
											PushString(GetTextStart(n->text), false, memory);
										PushString(Null, true, memory);
									}
								}
								EndNotesLoop();
							
								SaveFile(memory.memory, memory.size, path);
								Win32DisplayInfoBox("File saved!", false);
								
								FreeStack(memory);
							}
						}
						else if(it == 1) { // Load
							// Set correct directory
							if(StringsAreEqual(Win32GetCurrentDirectory(), "Boards") == false) {
								if(Win32DirectoryExists("Boards") == false) {
									Win32DisplayInfoBox("No files to load yet", false);
									goto label_rendering; // Nothing to open if the directory didn't even exist
								}
								Win32SetCurrentDirectory("Boards");
							}
							
							char* path = OpenFileGUI(".\\Boards", "Bola boards\0*.bb\0\0"); // Get open file path
							if(path != Null && FileExists(path) == true) {
								auto file = LoadFile(path);
								void* data = file.memory;
								
								char* boardTitle = (char*)data;
								MovePtr(data, GetStringLength(boardTitle) + 1);
								if(boardTitle[0] != '\0') {
									ClearTextBody(state.titleBar.text);
									InsertText(boardTitle, GetStringLength(boardTitle), state.titleBar.text, 0);
								}
								
								ClearMemory(state.notes.memory.memory, state.notes.memory.size);
								
								// Extract notes
								while(data < (ui8*)file.memory + file.size) {
									f32 left = ReadMemMovePtr(data, f32);
									f32 bottom = ReadMemMovePtr(data, f32);
									f32 width = ReadMemMovePtr(data, f32);
									f32 height = ReadMemMovePtr(data, f32);
									
									char* title = (char*)data;
									MovePtr(data, GetStringLength(title) + 1);
									
									char* text = (char*)data;
									MovePtr(data, GetStringLength(text) + 1);
									
									auto* n = CreateNote({left, bottom}, title[0] == '\0' ? Null : title, text[0] == '\0' ? Null : text);
									n->background.width = width;
									n->background.height = height;
								}
							}
						}
						
						goto label_rendering;
					}
				}
			}
		}
		
		// Selection of the title bar
		if(osState.mouseLeftDoubleClick == true && MouseOverlapsGUI(state.titleBar.background) == true) {
			if(TextIsBeingWritten() == true && TitleIsBeingUpdated() == false)
				EndWriting();
			state.notes.selected = Null;
			BeginWriting(state.titleBar.text, &state.titleBar.background, TitleBarTextHeight, false);
			goto label_rendering;
		}
		
		// Toolbar
		if(MouseLeftDownThisFrame() == true && MouseOverlapsGUI(state.toolBar.buttons[0].background) == true) { // Create new note
			auto pos = ConvertToCanvasSpace(0, osState.mouseY - NoteMinHeight / 2);
			auto* n = CreateNote(pos, Null, Null);
			state.notes.selected = n;
			state.notes.justCreated = true;
			state.notes.moving = true;
	
			goto label_rendering;
		}
		else if(TextIsBeingWritten() && MouseLeftDownThisFrame() == true && MouseOverlapsGUI(state.toolBar.buttons[1].background) == true && state.textUpdate.textBody->specialCharsAllowed == true) { // Add a bullet point
			Assert(state.textUpdate.textBody != Null);
			char* text = GetTextStart(*state.textUpdate.textBody);
			auto  length = GetStringLength(text);
			Assert(state.textUpdate.cursorOffset <= length);
			if(state.textUpdate.cursorOffset == 0 || state.textUpdate.cursorOffset >= 1 && text[state.textUpdate.cursorOffset - 1] != '\b') // If the char at the cursor position is not a bullet point
				AddText('\b');
				
			goto label_rendering;
		}
		else if(GetCurrentNote() != Null && MouseLeftDownThisFrame() == true && MouseOverlapsGUI(state.toolBar.buttons[2].background) == true) { // Add a title to the current note
			auto* n = GetCurrentNote();
			if(NoteHasTitle(n) == false) {
				n->title = AllocateTextBody(false);
				InsertText("Title", GetStringLength("Title"), n->title, 0);
				n->background.bottom -= NoteTextBorder * 2 + NoteTitleTextHeight;
				n->background.height += NoteTextBorder * 2 + NoteTitleTextHeight;
			}
		
			goto label_rendering;
		}
		else if(MouseLeftDownThisFrame() == true && MouseOverlapsGUI(state.toolBar.buttons[3].background) == true) { // Toggle colour wheel
			auto* panel = GetColourPanel();
			panel->display = !panel->display;
			if(panel->display == true) {
				panel->frame.left = GetTopRight(GetToolBar()->background).x + 100;
				panel->frame.width = ColourPanelWidth;
				panel->frame.height = ColourPanelHeight;
				panel->frame.bottom = GetMiddle(GetToolBar()->buttons[3].background).y - panel->frame.height / 2;
				panel->selection = GetMiddle(GetColourPanelWheelRectangle());
				panel->sliderCenterY = GetTopRight(GetColourPanelWheelRectangle()).y;
			}
			goto label_rendering;
		}
		
		// Colour panel
		if(MouseLeftDownThisFrame() == true && MouseOverlapsGUI(GetColourPanel()->frame) == true) {
			auto* panel = GetColourPanel();
			auto wheel = GetColourPanelWheelRectangle();
			if(Overlap(GetMiddle(wheel).x, GetMiddle(wheel).y, state.mouse.pos.x, state.mouse.pos.y, wheel.width / 2) == true) { // Colour wheel
				panel->updatingSelection = true;
				panel->selection = state.mouse.pos;
			}
			else if(MouseOverlapsGUI(GetColourPanelSliderRectangle()) == true) { // Colour slider
				panel->updatingSlider = true;
				panel->sliderCenterY = state.mouse.pos.y;
			}
			
			goto label_rendering;
		}
		else if(GetColourPanel()->updatingSelection == true) { // Update selection position
			auto* panel = GetColourPanel();
			if(osState.mouseLeftClickUp == true)
				panel->updatingSelection = false;
			else {
				auto newPos = panel->selection + state.mouse.translation;
				auto wheel = GetColourPanelWheelRectangle();
				if(Overlap(GetMiddle(wheel).x, GetMiddle(wheel).y, newPos.x, newPos.y, wheel.width / 2) == true) // Check if new position lies within the wheel
					panel->selection += state.mouse.translation;
			}
		}
		else if(GetColourPanel()->updatingSlider == true) { // Update slider position
			auto* panel = GetColourPanel();
			if(osState.mouseLeftClickUp == true)
				panel->updatingSlider = false;
			else {
				auto newPos = panel->selection + state.mouse.translation;
				auto rec = GetColourPanelSliderRectangle();
				if(MouseOverlapsGUI(GetColourPanelSliderRectangle()) == true) // Check if new position lies within the wheel
					panel->sliderCenterY += state.mouse.translation.y;
			}
		}

		// Notes
		if(osState.mouseLeftDoubleClick == true) { // Begin writing regardles of whether a note is selected @TODO - Technically a note would have already been selected be 1st mouse click, simplify?
			auto* previouslySelected = state.notes.selected;
			state.notes.selected = Null;
		
			BeginNotesLoop(n) {
				if(NoteMemoryIsInUse(n) == true && MouseOverlapsCanvas(n->background) == true) {
					state.notes.selected = n;
					state.notes.moving = true;
					break;
				}
			}
			EndNotesLoop();
			
			// If we selected a different note or text was being written elsewhere, end writing first
			if(TextIsBeingWritten() == true && (state.notes.selected == Null || previouslySelected != state.notes.selected))
				EndWriting();
			
			if(state.notes.selected != Null) {
				auto mousePosCanvas = ConvertToCanvasSpace(state.mouse.pos);
				auto renderData = GetNoteTextRenderData(state.notes.selected);
				if(MouseOverlapsCanvas(renderData.titleContainer) == true) { // Update title
					BeginWriting(state.notes.selected->title, &state.notes.selected->background, NoteTitleTextHeight, false);
					
					// Position mouse cursor more precisely
					f32 offset = (mousePosCanvas.x - renderData.title.left) / (NoteTitleTextHeight * 1.5f); // @TODO - This takes into consideration a small space of length NoteTitleTextHeight * 0.5f at the end of the text
					offset = RoundToNearestInteger(offset);
					Clamp(offset, 0, GetTextLength(state.notes.selected->title));
					state.textUpdate.cursorOffset = offset;
				}
				else if(MouseOverlapsCanvas(renderData.textContainer) == true) { // Update text
					auto* n = state.notes.selected;
					
					BeginWriting(n->text, &n->background, NoteTextHeight, true);
					
					// Loop through the text and check each glyphs's position agains mouse pos to correctly set the cursor
					auto* text = GetTextStart(n->text);
					auto  length = GetTextLength(n->text);
					f32   x = renderData.text.left;
					f32   y = renderData.text.bottom + renderData.text.height - NoteTextHeight;
					ForAll(length) {\
						if(text[it] == '\n') {
							y -= NoteTextHeight * 1.5f; 
							x = renderData.text.left;
						}
						else if(mousePosCanvas.y >= y && mousePosCanvas.y <= y + NoteTextHeight * 1.5f) { // Check for vertical overlap
							// Get the line end
							char* end = FindChar('\n', it, true);
							if(end == Null) // Very last line
								end = text + length;
								
							ui16 width = GetTextRenderDimensions(text + it, (ui32)(end - (text + it)), NoteTextHeight).x;
							if(mousePosCanvas.x <= x + width) // If mouse is within a line
								state.textUpdate.cursorOffset = it + (mousePosCanvas.x - renderData.text.left) / (NoteTextHeight * 1.5f);
							else
								state.textUpdate.cursorOffset = end - text; // Place cursor at the end of the line
							break;
						}
						else
							x += NoteTextHeight * 1.5f;
					}
				}
			}
		}
		else if(MouseLeftDownThisFrame() == true) { // Select
			auto* previouslySelected = state.notes.selected;
			state.notes.selected = Null;
		
			auto mousePosCanvas = ConvertToCanvasSpace(state.mouse.pos.x, state.mouse.pos.y);
			BeginNotesLoop(n) {
				if(NoteMemoryIsInUse(n) == true && MouseOverlapsCanvas(n->background) == true) {
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
		else if(state.notes.moving == true && state.mouse.leftDown == true) { // Move
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
		else if(state.notes.moving == true && state.mouse.leftDown == false) { // Drop
			state.notes.moving = false;
			
			// If the note was just created, drop it outside of the toolbar
			Assert(state.notes.selected != Null);
			f32 toolbarEdgeCanvas = ConvertToCanvasSpace(state.toolBar.background.left + state.toolBar.background.width, Null).x;
			if(state.notes.selected->background.left < toolbarEdgeCanvas && state.notes.justCreated == true)
				state.notes.selected->background.left = toolbarEdgeCanvas;
			
			state.notes.justCreated = false;
		}
		else if(state.notes.selected != Null && TextIsBeingWritten() == false && (osState.deletePressed == true || osState.backspacePressed == true)) { // Delete note
			FreeTextBody(state.notes.selected->text);
			if(TextBodyIsValid(state.notes.selected->title) == true)
				FreeTextBody(state.notes.selected->title);
			ClearMemory(state.notes.selected, sizeof(note));
			state.notes.selected = Null;
		}
		
		// Update text somewhere
		if(TextIsBeingWritten() == true) {
			auto* tu = &state.textUpdate;
			
			if(osState.keyPressed != Null) // Push text
				AddText(osState.keyPressed);
			else if(osState.backspacePressed == true){
				bool del = tu->cursorOffset > 0; // If the cursor was already at 0, moving down would incorrectly deleted the very first letter
				MoveCursor(-1);
				if(del == true)
					RemoveChar(*tu->textBody, tu->cursorOffset);
			}
			else if(osState.enterPressed == true) { // Jump to next line if allowed, otherwise end writing
				if(tu->textBody->specialCharsAllowed == true) {
					// Scan back to see if the current line contains a bullet point
					bool  bulletPoint = false;
					char* text = GetTextStart(*tu->textBody);
					FromTo(tu->cursorOffset, 0) {
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
				else
					EndWriting();
			}
			else if(tu->cursorOffset >= 1 && GetTextStart(*tu->textBody)[tu->cursorOffset - 1] == '\b' && osState.tabPressed == true) // Remove bullet point if tab is pressed after it
				GetTextStart(*tu->textBody)[tu->cursorOffset - 1] = ' ';
			else if(osState.escapePressed == true) { // Esc hit, end writing
				if(NoteIsBeingUpdated() == true)
					state.notes.selected = Null;
				EndWriting();
			}
			else if(osState.mouseLeftClickDown == true && MouseIsWithinToolbar() == false) { // Left mouse click outside of the tool bar, check where and decide
				vector mousePos = state.mouse.pos;
				if(TitleIsBeingUpdated() == false) // If it's not the title bar, check for overlap in canvas space
					mousePos = ConvertToCanvasSpace(state.mouse.pos.x, state.mouse.pos.y);
				if(Overlap(mousePos.x, mousePos.y, UnpackRectangle(*tu->containerBackground)) == false)
					EndWriting();
			}
			else if(osState.leftPressed == true && tu->cursorOffset >= 1)
				MoveCursor(-1);
			else if(osState.rightPressed == true && tu->cursorOffset < GetTextLength(*tu->textBody))
				MoveCursor(1);
			else if(osState.downPressed == true && NoteIsBeingUpdated() == true) {
				auto* n = GetCurrentNote();
				
				if(NoteHasTitle(n) == true && tu->textBody == &n->title) { // If we're updating a title, go to text section
					auto previousCursorOffset = tu->cursorOffset;
					
					EndWriting();
					BeginWriting(n->text, &n->background, NoteTextHeight, true);
					
					auto renderData = GetNoteTextRenderData(n);
					Assert(previousCursorOffset <= GetTextLength(n->title));
					f32 cursorX = renderData.title.left + GetTextRenderDimensions(GetTextStart(n->title), previousCursorOffset, NoteTitleTextHeight).x + NoteTitleTextHeight * 0.25f;
					f32 cursorIndex = (cursorX - renderData.text.left) / (NoteTextHeight * 1.5f);
					cursorIndex = RoundToNearestInteger(cursorIndex); // @TODO - Export to APAD API?
					Assert(cursorIndex > 0);
					
					tu->cursorOffset = GetMin(cursorIndex, GetTextLength(n->text));
				}
				else { // Move down one line within note text
					// Need to scan behind and in front of the cursor to determine the bounds of the current line
					char* end = FindChar('\n', tu->cursorOffset, true);
					if(end != Null) {
						ui32  lineStartOffset = tu->cursorOffset;
						char* start = FindChar('\n', tu->cursorOffset, false);
						if(start != Null)
							lineStartOffset -= (ui8*)start + 1 - (ui8*)GetTextStart(*tu->textBody);
						
						ui32 delta = (ui32)((ui8*)end + 1 - tu->cursorOffset + lineStartOffset);
						MoveCursor(delta);
					}
				}
			}
			else if(osState.upPressed == true && NoteIsBeingUpdated() == true) {
				auto* n = GetCurrentNote();
				
				char* start = FindChar('\n', tu->cursorOffset, false);
				if(start == Null && NoteHasTitle(n) == true && tu->textBody == &n->text) { // If we're updating the first line of text and note has a title, move to the latter
					auto previousCursorOffset = tu->cursorOffset;
					
					EndWriting();
					BeginWriting(n->title, &n->background, NoteTitleTextHeight, false);
					
					auto renderData = GetNoteTextRenderData(n);
					Assert(previousCursorOffset <= GetTextLength(n->text));
					f32 cursorX = renderData.text.left + GetTextRenderDimensions(GetTextStart(n->text), previousCursorOffset, NoteTextHeight).x + NoteTextHeight * 0.25f;
					f32 cursorIndex = 0;
					if(cursorX > renderData.title.left && cursorX < renderData.title.left + renderData.title.width) {
						cursorIndex = (cursorX - renderData.title.left) / (NoteTitleTextHeight * 1.5f);
						cursorIndex = RoundToNearestInteger(cursorIndex); // @TODO - Export to APAD API?
					}
					else if(cursorX >= renderData.title.left + renderData.title.width)
						cursorIndex = GetTextLength(n->title);
					Assert(cursorIndex >= 0);
					
					tu->cursorOffset = GetMin(cursorIndex, GetTextLength(n->title));
				}
				else if(start != Null) { // Move up one line within note text
					// Need to scan behind and in front of the cursor to determine the bounds of the current line
					if(start != Null) {
						bool  previousLineIsLonger = false;
						char* previousLineStart = FindChar('\n', GetCharOffset(start), false);
						if(previousLineStart == Null) // The line above is the very first one
							previousLineIsLonger = GetCharOffset(start) > tu->cursorOffset - GetCharOffset(start + 1); 
						else { // The line above is at least the second in the paragraph
							auto lineLength = GetCharOffset(start) - GetCharOffset(previousLineStart + 1);
							previousLineIsLonger = lineLength > tu->cursorOffset - GetCharOffset(start + 1);
						}
						
						if(previousLineIsLonger == true) { // Just move cursor up
							ui16 cursorOffset = tu->cursorOffset - GetCharOffset(start + 1);
							ui16 lineStartIndex = previousLineStart == Null ? 0 : GetCharOffset(previousLineStart + 1);
							tu->cursorOffset = lineStartIndex + cursorOffset;
						}
						else // Place cursor at the end of the previous line
							MoveCursor(GetCharOffset(start) - tu->cursorOffset);
					}
				}
			}
			
			// Update cursor position
			if(TextIsBeingWritten() == true) { // In case EndWriting() was called above
				vector pos = {};
				{
					Assert(tu->cursorOffset <= GetTextLength(*tu->textBody));
					auto* text = GetTextStart(*tu->textBody);
					ForAll(tu->cursorOffset) {
						if(text[it] == '\n') {
							pos.x = 0;
							pos.y -= tu->textHeight * 1.5f;
						}
						else
							pos.x += tu->textHeight * 1.5f;
					}
					if(tu->cursorOffset > 0)
						pos.x -= tu->textHeight * 0.25f; // Place half way between 2 glyphs
				}
				
				if(TitleIsBeingUpdated() == true) { // Writing on the title bar
					auto fullLength = GetTextRenderDimensions(GetTextStart(*tu->textBody), GetTextLength(*tu->textBody), TitleBarTextHeight).x;
					pos.x += GetMiddle(state.titleBar.background).x - fullLength / 2;
					pos.y += GetMiddle(state.titleBar.background).y - TitleBarTextHeight / 2;
				}
				else { // Writing on a note
					Assert(NoteIsBeingUpdated() == true);
					auto* n = GetCurrentNote();
					auto  renderData = GetNoteTextRenderData(n);
					if(NoteHasTitle(n) == true && tu->textBody == &n->title) { // Updating title
						pos.x += renderData.title.left;
						pos.y += renderData.title.bottom;
					}
					else { // Update text
						pos.x += renderData.text.left;
						pos.y += renderData.textContainer.bottom + renderData.textContainer.height - NoteTextHeight; // Starts at the top
					}
				}
				
				SetCursorPos(pos.x, pos.y);
			}
			
			// Update cursor blink animation
			tu->cursorBlinkTime += osState.lastFrameTime;
			if(tu->cursorBlinkTime > CursorBlinkFullLength) {
				// In case we get a frame time >= CursorBlinkFullLength * 2 for whatever reason
				do 		tu->cursorBlinkTime -= CursorBlinkFullLength;
				while(tu->cursorBlinkTime > CursorBlinkFullLength);
			}
			Assert(tu->cursorBlinkTime >= 0);
			Assert(tu->cursorBlinkTime <= CursorBlinkFullLength);
			if(tu->cursorBlinkTime >= 0 && tu->cursorBlinkTime < CursorBlinkFullLength / 2)
				tu->cursorAlpha = LERP(1.0f, 0.0f, tu->cursorBlinkTime / (CursorBlinkFullLength / 2));
			else
				tu->cursorAlpha = LERP(0.0f, 1.0f, (tu->cursorBlinkTime - CursorBlinkFullLength / 2) / (CursorBlinkFullLength / 2)); 
			Assert(tu->cursorAlpha >= 0);
			Assert(tu->cursorAlpha <= 1.0f);
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
					DrawRectangle(UnpackRectangle(n->background), 255, 255, 255);
			}
		}
		EndNotesMemoryLoop();

		// Draw border on a selected note
		if(state.notes.selected != Null && state.notes.justCreated == false)
			DrawRectangleBorder(UnpackRectangle(state.notes.selected->background), 0, 0, 0);

		// @TODO - Is Win32GetMousePoswidthinClient() needed anymore?

		// Render notes text and title if it has one, then update note background rectangle
		BeginNotesLoop(n) {
			if(NoteMemoryIsInUse(n) == true) {
				rectangle textBox = {};
				rectangle titleBox = {};
				auto      renderData = GetNoteTextRenderData(n);
				if(GetTextLength(n->text) > 0) // Note has text
					textBox = RenderText(GetTextStart(n->text), GetTextLength(n->text), renderData.text.left, renderData.text.bottom + renderData.text.height - NoteTextHeight, NoteTextHeight, false);
				if(NoteHasTitle(n) == true && GetTextLength(n->title) > 0)
					titleBox = RenderText(GetTextStart(n->title), GetTextLength(n->title), renderData.title.left, renderData.title.bottom, NoteTitleTextHeight, false);
				
				// Resize note
				if(TextIsBeingWritten() == true && state.textUpdate.containerBackground == &n->background) {
					n->background.width = GetMax(NoteMinWidth, textBox.width + NoteTextBorder * 2);
					f32 top = n->background.bottom + n->background.height;
					n->background.height = GetMax(NoteMinHeight, textBox.height + NoteTextBorder * 2);
					if(NoteHasTitle(n) == true) {
						n->background.width = GetMax(n->background.width, titleBox.width + NoteTextBorder * 2);
						n->background.height += NoteTextBorder * 2 + NoteTitleTextHeight;
					}
					n->background.bottom = top - n->background.height;
				}
			}
		}
		EndNotesLoop();
		
		// Draw the overlying UI
		SetGUIProjectionMatrix();
			
		// Toolbar
		{
			auto* tb = &state.toolBar;
			DrawRectangle(UnpackRectangle(state.toolBar.background), 255, 255, 255); // Background
	
			ForAll(GetArrayLength(tb->buttons)) { // Buttons
				auto* b = tb->buttons + it;
				DrawRectangle(UnpackRectangle(b->background), 255, 0, 0);
				RenderText((char*)b->text, GetStringLength(b->text), GetMiddle(b->background).x, b->textBottom, tb->textHeight, true);
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
			DrawRectangle(UnpackRectangle(tb->background), 255, 255, 255);
			if(GetTextLength(tb->text) > 1) {
				auto middle = GetMiddle(tb->background);
				RenderText(GetTextStart(tb->text), GetTextLength(tb->text), middle.x, middle.y - TitleBarTextHeight / 2, TitleBarTextHeight, true);
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
			DrawRectangle(UnpackRectangle(state.notes.selected->background), 255, 255, 255);
			DrawRectangleBorder(UnpackRectangle(state.notes.selected->background), 0, 0, 0);
		}
		
		// Draw cursor if needed
		if(state.textUpdate.textBody != Null) {
			auto* tu = &state.textUpdate;
			
			SetGUIProjectionMatrix();
			if(TitleIsBeingUpdated() == false)
				SetCanvasProjetionMatrix();
			
			glLineWidth(2);
			glColor4f(0, 0, 0, tu->cursorAlpha);
			glBegin(GL_LINES);
			glVertex2f(tu->cursorPos.x, tu->cursorPos.y);
			glVertex2f(tu->cursorPos.x, tu->cursorPos.y+ tu->textHeight);
			glEnd();
			AssertOpenGL();
		}
		
		// Render top menu
		{
			SetGUIProjectionMatrix();
			
			auto* m = &state.topMenu;
			DrawRectangle(UnpackRectangle(m->background), 146, 139, 183);
		
			// Potentially highlight selected option
			if(MouseOverlapsGUI(m->background) == true) {
				FromToInc(GetArrayLength(m->buttons) - 1, 0) {
					if(state.mouse.pos.x >= m->buttons[it].left && state.mouse.pos.x < m->buttons[it].left + TopMenuButtonWidth) {
						DrawRectangle(m->buttons[it].left, m->background.bottom, TopMenuButtonWidth, m->background.height, 0, 0, 0);
						OutputDebugString(Concatenate(2, "\n ", ToString(state.mouse.pos.y)));
						break;
					}
				}
			}
			
			ForAll(GetArrayLength(m->buttons)) {
				auto* b = m->buttons + it;
				RenderText((char*)b->text, GetStringLength(b->text), b->left + TopMenuButtonWidth / 2, GetMiddle(m->background).y - TopMenuTextHeight / 2, TopMenuTextHeight, true);
				
				// Draw separator
				if(it < GetArrayLength(m->buttons) - 1) {
					glColor3f(1, 1, 1);
					glLineWidth(2);
					glBegin(GL_LINES);
					glVertex2f(b->left + TopMenuButtonWidth, GetMiddle(m->background).y - TopMenuTextHeight / 2);
					glVertex2f(b->left + TopMenuButtonWidth, GetMiddle(m->background).y + TopMenuTextHeight / 2);
					glEnd();
					AssertOpenGL();
				}
			}
		}
		
		// Colour panel
		if(GetColourPanel()->display == true) {
			auto* panel = GetColourPanel();
			
			SetGUIProjectionMatrix();
			DrawRectangle(UnpackRectangle(panel->frame), 255, 255, 255); // Draw the frame
			glLineWidth(2);
			DrawRectangleBorder(UnpackRectangle(panel->frame), 0, 0, 0);
			
			// Draw colour wheel
			{
				auto r = GetColourPanelWheelRectangle();
				glBegin(GL_TRIANGLE_FAN);
				
				// Center
				glColor3f(1.0f, 1.0f, 1.0f);
				glVertex2f(GetMiddle(r).x, GetMiddle(r).y);
				
				// Draw the wheel itself
				FromToInc(0, ColourWheelVertices + 1) {
					f32 angle = it * 360 / ColourWheelVertices;
					if(angle <= 120)
						glColor3f(LERP(1.0f, 0, angle / 120), LERP(0, 1.0f, angle / 120), 0);
					else if(angle <= 240)
						glColor3f(0, LERP(1.0f, 0, (angle - 120) / 120), LERP(0, 1.0f, (angle - 120) / 120));
					else
						glColor3f(LERP(0, 1.0f, (angle - 240) / 120), 0, LERP(1.0f, 0, (angle - 240) / 120));
					f32 x = GetMiddle(r).x - Sine(angle) * r.width / 2;
					f32 y = GetMiddle(r).y + Cos(angle) * r.height / 2;
					glVertex2f(x, y);
				}
				glEnd();
				
				// Draw outer edges
				DrawCircleBorder(GetMiddle(r).x, GetMiddle(r).y, r.width / 2, 2, 0, 0, 0);
				
				// Draw selection
				{
					// @TODO - Draw a circle instead of a rectangle
					f32 size = 10;
					glLineWidth(1);
					DrawRectangleBorder(panel->selection.x - size / 2, panel->selection.y - size / 2, size, size, 255, 255, 255);
				}
				
				AssertOpenGL();
			}
			
			// Get the current wheel colour selection
			f32 wheelRed = 0;
			f32 wheelGreen = 0;
			f32 wheelBlue = 0;
			{
				auto wheelRec = GetColourPanelWheelRectangle();
				auto vector = panel->selection - GetMiddle(wheelRec);
				
				// Scale magnitude
				f32 magnitude01 = Magnitude(vector) / (wheelRec.width / 2); // 0 -> 1 between circle center and outer edges
				
				// Angle of current selection
				f32 angle = 0; // About the horizontal axis
				if(vector.x == 0) 
					angle = vector.y > 0 ? 90 : 270;
				else if(vector.y == 0)
					angle = vector.x > 0 ? 0 : 180;
				else {
					f32 a = Magnitude(vector.x);
					f32 o = Magnitude(vector.y);
					angle = ArcTan(o / a);
					if(vector.x < 0 && vector.y > 0)
						angle = 180 - angle;
					else if(vector.x < 0 && vector.y < 0)
						angle += 180;
					else if(vector.x > 0 && vector.y < 0)
						angle = 360 - angle;
				}
				
				// Adjust angle to start from the vertical axis (red)
				angle -= 90;
				if(angle < 0)
					angle += 360;
				
				// Work out the max colour based on the angle
				f32 rmax = 0;
				f32 gmax = 0;
				f32 bmax = 0;
				if(angle <= 120) {
					rmax = LERP(1.0f, 0, angle / 120);
					gmax = LERP(0, 1.0f, angle / 120);
				}
				else if(angle <= 240) {
					gmax = LERP(1.0f, 0, (angle - 120) / 120);
					bmax = LERP(0, 1.0f, (angle - 120) / 120);
				}
				else {
					rmax = LERP(0, 1.0f, (angle - 240) / 120);
					bmax = LERP(1.0f, 0, (angle - 240) / 120);
				}
				
				// Final colour
				wheelRed = LERP(1.0f, rmax, magnitude01);
				wheelGreen = LERP(1.0f, gmax, magnitude01);
				wheelBlue = LERP(1.0f, bmax, magnitude01);
			}
			
			// Draw colour slider to the right of the wheel
			{
				auto rec = GetColourPanelSliderRectangle();
				glBegin(GL_QUADS);
				glColor3f(0, 0, 0);
				glVertex2f(rec.left, rec.bottom);
				glVertex2f(rec.left + rec.width, rec.bottom);
				glColor3f(wheelRed, wheelGreen, wheelBlue);
				glVertex2f(rec.left + rec.width, rec.bottom + rec.height);
				glVertex2f(rec.left, rec.bottom + rec.height);
				glEnd();
				glLineWidth(1);
				DrawRectangleBorder(rec.left, rec.bottom, rec.width, rec.height, 0, 0, 0);
				
				// Draw selection
				DrawRectangleBorder(rec.left - 3, panel->sliderCenterY - 5, rec.width + 6, 10, 0, 0, 0);
			}
			
			// Bottom half - sample colour and rgb text boxes
			{
				auto* panel = GetColourPanel();
				
				// For now just draw 1 circle and 3 boxes
				ForAll(4) {
					if(it == 0) { // Final colour selection
						auto rec = GetColourPanelSliderRectangle();
						f32 sliderScale = (panel->sliderCenterY - rec.bottom) / rec.height;
						f32 r = wheelRed * sliderScale;
						f32 g = wheelGreen * sliderScale;
						f32 b = wheelBlue * sliderScale;
						
						f32 centerX = panel->frame.left + (panel->frame.width / 4) * it + panel->frame.width / 8;
						f32 centerY = panel->frame.bottom + panel->frame.height / 4;
						f32 radius = panel->frame.width / 5 / 2;
						
						//  Colour circle
						glBegin(GL_TRIANGLE_FAN);
						glColor3f(r, g, b);
						ui8 vertices = 36;
						FromToInc(0, vertices + 1) {
							f32 angle = it * 360 / vertices;
							f32 x = centerX - Sine(angle) * radius;
							f32 y = centerY + Cos(angle) * radius;
							glVertex2f(x, y);
						}
						glEnd();
						
						// Frame
						DrawCircleBorder(centerX, centerY, radius, 2, 0, 0, 0);
					}
					else {
						f32 middleX = panel->frame.left + (panel->frame.width / 4) * it + panel->frame.width / 8;
						f32 width = panel->frame.width / 5;
						f32 middleY = panel->frame.bottom + panel->frame.height / 4;
						f32 height = panel->frame.height / 5;
						glLineWidth(1);
						DrawRectangleBorder(middleX - width / 2, middleY - height / 2, width, height, 0, 0, 0);
					}
				}
			}
		}
		
		// Store state before next frame
		state.mouse.lastLeftDown = state.mouse.leftDown;

		Win32EndGUIUpdateLoop();
	}
	
	return 0;
}