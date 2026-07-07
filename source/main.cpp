#include <windows.h>
#include <gl\gl.h>
#include "apad_array.h"
#include "apad_base_types.h"
#include "apad_error.h"
#include "apad_file.h"
#include "apad_gui.h"
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
		tb->text = AllocateTextBody(TitleBarTextHeight, TextBodyFlagLetters);
		InsertString("Title", GetStringLength("Title"), tb->text, 0);
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
			DrawRectangleFull(0, 0, canvas.width, canvas.height, 230, 230, 230);
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
								if(GetTextBodyLength(state.titleBar.text) > 0)
									PushString(GetTextBodyText(state.titleBar.text), false, memory);
								PushString(Null, true, memory);
								
								// Notes
								BeginNotesLoop(n) {
									if(NoteMemoryIsInUse(n) == true) {
										// Store the pos
										f32* f = PushType(f32, memory);
										*f = n->pos.x;
										f = PushType(f32, memory);
										*f = n->pos.y;
										
										// Text contents
										if(NoteHasTitle(n) == true)
											PushString(GetTextBodyText(n->title), false, memory);
										PushString(Null, true, memory);
										
										if(GetTextBodyLength(n->text) > 0)
											PushString(GetTextBodyText(n->text), false, memory);
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
									InsertString(boardTitle, GetStringLength(boardTitle), state.titleBar.text, 0);
								}
								
								ClearMemory(state.notes.memory.memory, state.notes.memory.size);
								
								// Extract notes
								while(data < (ui8*)file.memory + file.size) {
									f32 x = ReadMemMovePtr(data, f32);
									f32 y = ReadMemMovePtr(data, f32);
									
									char* title = (char*)data;
									MovePtr(data, GetStringLength(title) + 1);
									
									char* text = (char*)data;
									MovePtr(data, GetStringLength(text) + 1);
									
									auto* n = CreateNote(CreateVector(x, y), title[0] == '\0' ? Null : title, text[0] == '\0' ? Null : text);
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
			if(TextIsBeingUpdated() == true && TitleIsBeingUpdated() == false)
				EndTextUpdate();
			SetCurrentNote(Null);
			BeginTextUpdate(state.titleBar.text);
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
		else if(NoteIsBeingUpdated() == true && MouseLeftDownThisFrame() == true && MouseOverlapsGUI(state.toolBar.buttons[1].background) == true) { // Add a bullet point
			InsertCharAtCursor(BulletPointChar); // Will check viability first	
			goto label_rendering;
		}
		else if(GetCurrentNote() != Null && MouseLeftDownThisFrame() == true && MouseOverlapsGUI(state.toolBar.buttons[2].background) == true) { // Add a title to the currently selected note
			auto* n = GetCurrentNote();
			if(NoteHasTitle(n) == false) {
				n->title = AllocateTextBody(NoteTitleTextHeight, TextBodyFlagLetters);
				InsertString("Title", GetStringLength("Title"), n->title, 0);
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
				panel->frame.bottom = GetCenter(GetToolBar()->buttons[3].background).y - panel->frame.height / 2;
				panel->selection = GetCenter(GetColourPanelWheelRectangle());
				panel->sliderCenterY = GetTopRight(GetColourPanelWheelRectangle()).y;
				panel->red = AllocateTextBody(NoteTextHeight, Null);
				panel->green = AllocateTextBody(NoteTextHeight, Null);
				panel->blue = AllocateTextBody(NoteTextHeight, Null);
			}
			else {
				FreeTextBody(panel->red);
				FreeTextBody(panel->green);
				FreeTextBody(panel->blue);
			}
			goto label_rendering;
		}
		
		// Colour panel
		if(MouseLeftDownThisFrame() == true && MouseOverlapsGUI(GetColourPanel()->frame) == true) {
			auto* panel = GetColourPanel();
			auto wheel = GetColourPanelWheelRectangle();
			if(Overlap(GetCenter(wheel).x, GetCenter(wheel).y, state.mouse.pos.x, state.mouse.pos.y, wheel.width / 2) == true) { // Colour wheel
				panel->updatingSelection = true;
				panel->selection = state.mouse.pos;
			}
			else if(MouseOverlapsGUI(GetColourPanelSliderRectangle()) == true) { // Colour slider
				panel->updatingSlider = true;
				panel->sliderCenterY = state.mouse.pos.y;
			}
			else {
				
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
				if(Overlap(GetCenter(wheel).x, GetCenter(wheel).y, newPos.x, newPos.y, wheel.width / 2) == true) // Check if new position lies within the wheel
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
			auto* previouslySelected = GetCurrentNote();
			SetCurrentNote(Null);
		
			BeginNotesLoop(n) {
				auto vectors = GetNoteTextVectors(n);
				if(NoteMemoryIsInUse(n) == true && (MouseOverlapsCanvas(vectors.textContainer) == true || MouseOverlapsCanvas(vectors.titleContainer) == true)) {
					SetCurrentNote(n);
					state.notes.moving = false;
					break;
				}
			}
			EndNotesLoop();
			
			// If we selected a different note or text was being written elsewhere, end writing first
			if(TextIsBeingUpdated() == true && (GetCurrentNote() == Null || previouslySelected != GetCurrentNote()))
				EndTextUpdate();
			
			if(GetCurrentNote() != Null) {
				auto mousePosCanvas = ConvertToCanvasSpace(state.mouse.pos);
				auto vectors = GetNoteTextVectors(GetCurrentNote());
				if(MouseOverlapsCanvas(vectors.titleContainer) == true) { // Update title
					BeginTextUpdate(GetCurrentNote()->title);
					
					// Position mouse cursor more precisely
					f32 x = mousePosCanvas.x - vectors.titleEdges.left;
					SetCursor(x, 0);
				}
				else if(MouseOverlapsCanvas(vectors.textContainer) == true) { // Update text
					auto* n = GetCurrentNote();
					
					BeginTextUpdate(n->text);
					
					// Loop through the text and check each glyphs's position agains mouse pos to correctly set the cursor
					auto* text = GetTextBodyText(n->text);
					auto  length = GetTextBodyLength(n->text);
					f32   x = vectors.textEdges.left;
					f32   y = vectors.textEdges.bottom + vectors.textEdges.height - NoteTextHeight;
					ForAll(length) {\
						if(text[it] == '\n') {
							y -= NoteTextHeight * 1.5f; 
							x = vectors.textEdges.left;
						}
						else if(mousePosCanvas.y >= y && mousePosCanvas.y <= y + NoteTextHeight * 1.5f) { // Check for vertical overlap
							// Get the line end
							char* end = FindChar('\n', it, true);
							if(end == Null) // Very last line
								end = text + length;
								
							ui16 width = GetTextRenderDimensions(text + it, (ui32)(end - (text + it)), NoteTextHeight).x;
							if(mousePosCanvas.x <= x + width) // If mouse is within a line
								state.textUpdate.cursorCharOffset = it + (mousePosCanvas.x - vectors.textEdges.left) / (NoteTextHeight * 1.5f);
							else
								state.textUpdate.cursorCharOffset = end - text; // Place cursor at the end of the line
							break;
						}
						else
							x += NoteTextHeight * 1.5f;
					}
				}
			}
		}
		else if(MouseLeftDownThisFrame() == true) { // Select
			auto* previouslySelected = GetCurrentNote();
			SetCurrentNote(Null);
		
			auto mousePosCanvas = ConvertToCanvasSpace(state.mouse.pos.x, state.mouse.pos.y);
			BeginNotesLoop(n) {
				if(NoteMemoryIsInUse(n) == true && MouseOverlapsCanvas(n->background) == true) {
					SetCurrentNote(n);
					state.notes.moving = true;
					break;
				}
			}
			EndNotesLoop();
			
			// If we selected a different note or text was being written elsewhere, end writing
			if(TextIsBeingUpdated() == true && (GetCurrentNote() == Null || previouslySelected != GetCurrentNote()))
				EndTextUpdate();
		}
		else if(state.notes.moving == true && state.mouse.leftDown == true) { // Move
			Assert(state.notes.selected != Null);
			
			// Mouse moves in viewport space
			vector newPosCanvas = { GetCurrentNote()->background.left + (f32)state.mouse.translation.x / state.canvas.scale, 
														  GetCurrentNote()->background.bottom + (f32)state.mouse.translation.y / state.canvas.scale };
														 
			// When moving a new note outside of the toolbar, ensure it can't be moved back in
			if(state.notes.justCreated == true) {
				f32 toolbarEdgeCanvas = ConvertToCanvasSpace(state.toolBar.background.left + state.toolBar.background.width, Null).x;
				if(newPosCanvas.x >= toolbarEdgeCanvas)
					state.notes.justCreated = false;
			}
			
			GetCurrentNote()->background.left = newPosCanvas.x;
			GetCurrentNote()->background.bottom = newPosCanvas.y;
		}
		else if(state.notes.moving == true && state.mouse.leftDown == false) { // Drop
			state.notes.moving = false;
			
			// If the note was just created, drop it outside of the toolbar
			Assert(state.notes.selected != Null);
			f32 toolbarEdgeCanvas = ConvertToCanvasSpace(state.toolBar.background.left + state.toolBar.background.width, Null).x;
			if(GetCurrentNote()->background.left < toolbarEdgeCanvas && state.notes.justCreated == true)
				GetCurrentNote()->background.left = toolbarEdgeCanvas;
			
			state.notes.justCreated = false;
		}
		else if(state.notes.selected != Null && TextIsBeingUpdated() == false && (osState.deletePressed == true || osState.backspacePressed == true)) { // Delete note
			FreeTextBody(GetCurrentNote()->text);
			if(TextBodyIsValid(GetCurrentNote()->title) == true)
				FreeTextBody(GetCurrentNote()->title);
			ClearMemory(state.notes.selected, sizeof(note));
			SetCurrentNote(Null);
		}
		
		// Update text somewhere
		if(TextIsBeingUpdated() == true) {
			auto pipelineUpdate = RunTextUpdatePipeline(osState);
			
			if(osState.escPressed == true && NoteIsBeingUpdated() == true)
				SetCurrentNote(Null);
			else if(osState.mouseLeftClickDown == true && MouseIsWithinToolbar() == false) { // Left mouse click outside of the tool bar, check where and decide
				vector mousePos = state.mouse.pos;
				if(TitleIsBeingUpdated() == false) // If it's not the title bar, check for overlap in canvas space
					mousePos = ConvertToCanvasSpace(state.mouse.pos.x, state.mouse.pos.y);
				if(Overlap(mousePos.x, mousePos.y, UnpackRectangle(*tu->containerBackground)) == false)
					EndTextUpdate();
			}
			else if( // If we're updating a note title and want to move down, go to the text section
							pipelineUpdate.wantToLeaveTextBodyDown == true && NoteIsBeingUpdated() == true && NoteHasTitle(GetCurrentNote()) == true && GetCurrentTextBody() == &GetCurrentNote()->title) 
			{				
				auto* n = GetCurrentNote();
				auto  noteVectors = GetNoteTextVectors(n);
				f32   cursorXAbs = noteVectors.titleEdges.left + GetCursorPos().x - NoteTitleTextHeight * 0.25f; // @TODO - The 0.25f will probably change for each font
				
				EndTextUpdate();
				BeginTextUpdate(n->text);
				
				f32 xRel = cursorXAbs - noteVectors.textEdges.left;
				f32 yRel = noteVectors.textEdges.height;
				SetCursorPos(xRel, yRel);
				
				#if 0
				ui16 charOffset = cursorX / n->text.textHeight * 1.5f; // @TODO - For other fonts won't know the expect width of each glyph, which will change anyway for each one.
				
				
				f32 offset = GetTextRenderDimensions(GetTextBodyLength(n->text), 
				
				auto vectors = GetNoteTextVectors(n);
				Assert(previouscursorCharOffset <= GetTextBodyLength(n->title));
				f32 cursorX = GetCursorPos().x;
				f32 cursorIndex = cursorX / (NoteTextHeight * 1.5f);
				cursorIndex = RoundToNearestInteger(cursorIndex);
				Assert(cursorIndex > 0);
				
				tu->cursorCharOffset = GetMin(cursorIndex, GetTextBodyLength(n->text));
				#endif
			}
			else if(osState.upPressed == true && NoteIsBeingUpdated() == true) {
				auto* n = GetCurrentNote();
				
				char* start = FindChar('\n', tu->cursorCharOffset, false);
				if(start == Null && NoteHasTitle(n) == true && tu->textBody == &n->text) { // If we're updating the first line of text and note has a title, move to the latter
					auto previouscursorCharOffset = tu->cursorCharOffset;
					
					EndTextUpdate();
					BeginTextUpdate(n->title);
					
					auto vectors = GetNoteTextVectors(n);
					Assert(previouscursorCharOffset <= GetTextBodyLength(n->text));
					f32 cursorX = vectors.textEdges.left + GetTextRenderDimensions(GetTextBodyText(n->text), previouscursorCharOffset, NoteTextHeight).x + NoteTextHeight * 0.25f;
					f32 cursorIndex = 0;
					if(cursorX > vectors.titleEdges.left && cursorX < vectors.titleEdges.left + vectors.titleEdges.width) {
						cursorIndex = (cursorX - vectors.titleEdges.left) / (NoteTitleTextHeight * 1.5f);
						cursorIndex = RoundToNearestInteger(cursorIndex);
					}
					else if(cursorX >= vectors.titleEdges.left + vectors.titleEdges.width)
						cursorIndex = GetTextBodyLength(n->title);
					Assert(cursorIndex >= 0);
					
					tu->cursorCharOffset = GetMin(cursorIndex, GetTextBodyLength(n->title));
				}
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
					DrawRectangleFull(UnpackRectangle(n->background), 255, 255, 255);
			}
		}
		EndNotesMemoryLoop();

		// Draw border on a selected note
		if(state.notes.selected != Null && state.notes.justCreated == false)
			DrawRectangleBorder(UnpackRectangle(GetCurrentNote()->background), 0, 0, 0);

		// @TODO - Is Win32GetMousePoswidthinClient() needed anymore?

		// Render notes text and title if it has one, then update note background rectangle
		BeginNotesLoop(n) {
			if(NoteMemoryIsInUse(n) == true) {
				rectangle textBox = {};
				rectangle titleBox = {};
				auto      vectors = GetNoteTextVectors(n);
				if(GetTextBodyLength(n->text) > 0) // Note has text
					textBox = RenderText(GetTextBodyText(n->text), GetTextBodyLength(n->text), vectors.textEdges.left, vectors.textEdges.bottom + vectors.textEdges.height - NoteTextHeight, NoteTextHeight, false);
				if(NoteHasTitle(n) == true && GetTextBodyLength(n->title) > 0)
					titleBox = RenderText(GetTextBodyText(n->title), GetTextBodyLength(n->title), vectors.titleEdges.left, vectors.titleEdges.bottom, NoteTitleTextHeight, false);
				
				// Resize note
				if(TextIsBeingUpdated() == true && state.textUpdate.containerBackground == &n->background) {
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
			DrawRectangleFull(UnpackRectangle(state.toolBar.background), 255, 255, 255); // Background
	
			ForAll(GetArrayLength(tb->buttons)) { // Buttons
				auto* b = tb->buttons + it;
				DrawRectangleFull(UnpackRectangle(b->background), 255, 0, 0);
				RenderText((char*)b->text, GetStringLength(b->text), GetCenter(b->background).x, b->textBottom, tb->textHeight, true);
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
			DrawRectangleFull(UnpackRectangle(tb->background), 255, 255, 255);
			if(GetTextBodyLength(tb->text) > 1) {
				auto middle = GetCenter(tb->background);
				RenderText(GetTextBodyText(tb->text), GetTextBodyLength(tb->text), middle.x, middle.y - TitleBarTextHeight / 2, TitleBarTextHeight, true);
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
			DrawRectangleFull(UnpackRectangle(GetCurrentNote()->background), 255, 255, 255);
			DrawRectangleBorder(UnpackRectangle(GetCurrentNote()->background), 0, 0, 0);
		}
		
		// Draw cursor if needed
		if(state.textUpdate.textBody != Null) {
			auto* tu = &state.textUpdate;
			
			SetGUIProjectionMatrix();
			if(TitleIsBeingUpdated() == false)
				SetCanvasProjetionMatrix();
			
			// No need to store this value for now
			f32 alpha = 0;
			if(tu->cursorBlinkTime >= 0 && tu->cursorBlinkTime < CursorBlinkFullLength / 2)
				alpha = LERP(1.0f, 0.0f, tu->cursorBlinkTime / (CursorBlinkFullLength / 2));
			else
				alpha = LERP(0.0f, 1.0f, (tu->cursorBlinkTime - CursorBlinkFullLength / 2) / (CursorBlinkFullLength / 2)); 
			Assert(alpha >= 0);
			Assert(alpha <= 1.0f);
			
			glLineWidth(2);
			glColor4f(0, 0, 0, alpha);
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
			DrawRectangleFull(UnpackRectangle(m->background), 146, 139, 183);
		
			// Potentially highlight selected option
			if(MouseOverlapsGUI(m->background) == true) {
				FromToInc(GetArrayLength(m->buttons) - 1, 0) {
					if(state.mouse.pos.x >= m->buttons[it].left && state.mouse.pos.x < m->buttons[it].left + TopMenuButtonWidth) {
						DrawRectangleFull(m->buttons[it].left, m->background.bottom, TopMenuButtonWidth, m->background.height, 0, 0, 0);
						OutputDebugString(Concatenate(2, "\n ", ToString(state.mouse.pos.y)));
						break;
					}
				}
			}
			
			ForAll(GetArrayLength(m->buttons)) {
				auto* b = m->buttons + it;
				RenderText((char*)b->text, GetStringLength(b->text), b->left + TopMenuButtonWidth / 2, GetCenter(m->background).y - TopMenuTextHeight / 2, TopMenuTextHeight, true);
				
				// Draw separator
				if(it < GetArrayLength(m->buttons) - 1) {
					glColor3f(1, 1, 1);
					glLineWidth(2);
					glBegin(GL_LINES);
					glVertex2f(b->left + TopMenuButtonWidth, GetCenter(m->background).y - TopMenuTextHeight / 2);
					glVertex2f(b->left + TopMenuButtonWidth, GetCenter(m->background).y + TopMenuTextHeight / 2);
					glEnd();
					AssertOpenGL();
				}
			}
		}
		
		// Colour panel
		if(GetColourPanel()->display == true) {
			auto* panel = GetColourPanel();
			
			SetGUIProjectionMatrix();
			DrawRectangleFull(UnpackRectangle(panel->frame), 255, 255, 255); // Draw the frame
			glLineWidth(2);
			DrawRectangleBorder(UnpackRectangle(panel->frame), 0, 0, 0);
			
			// Draw colour wheel
			{
				auto r = GetColourPanelWheelRectangle();
				glBegin(GL_TRIANGLE_FAN);
				
				// Center
				glColor3f(1.0f, 1.0f, 1.0f);
				glVertex2f(GetCenter(r).x, GetCenter(r).y);
				
				// Draw the wheel itself
				FromToInc(0, ColourWheelVertices + 1) {
					f32 angle = it * 360 / ColourWheelVertices;
					if(angle <= 120)
						glColor3f(LERP(1.0f, 0, angle / 120), LERP(0, 1.0f, angle / 120), 0);
					else if(angle <= 240)
						glColor3f(0, LERP(1.0f, 0, (angle - 120) / 120), LERP(0, 1.0f, (angle - 120) / 120));
					else
						glColor3f(LERP(0, 1.0f, (angle - 240) / 120), 0, LERP(1.0f, 0, (angle - 240) / 120));
					f32 x = GetCenter(r).x - Sine(angle) * r.width / 2;
					f32 y = GetCenter(r).y + Cos(angle) * r.height / 2;
					glVertex2f(x, y);
				}
				glEnd();
				
				// Draw outer edges
				DrawCircleBorder(GetCenter(r).x, GetCenter(r).y, r.width / 2, 2, 0, 0, 0);
				
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
				auto vector = panel->selection - GetCenter(wheelRec);
				
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
					else { // RGB text boxes
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