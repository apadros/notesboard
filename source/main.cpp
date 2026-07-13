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
	state.titleBar = AllocateTextBody(0, state.topMenu.background.bottom - TitleBarHeight, Win32GetProgramWindowClientSize().width, (TitleBarHeight - TitleBarTextHeight) / 2, TitleBarTextHeight, TextBodyFlagLetters);
	Insert("Title", GetLength("Title"), state.titleBar, 0);
	
	// Init toolbar
	{
		auto* tb = &state.toolBar;
		tb->background.left = 0;
		tb->background.bottom = 0;
		tb->background.width = ToolbarWidth;
		tb->background.height = state.topMenu.background.bottom - GetTitleBar()->container.height;

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
							if(AreEqual(Win32GetCurrentDirectory(), "Boards") == false) {
								if(Win32DirectoryExists("Boards") == false)
									Win32CreateDirectory("Boards");
								Win32SetCurrentDirectory("Boards");
							}
							
							char* path = SaveFileAsGUI(Null, "Bola boards\0*.bb\0\0"); // Get save file path
							if(path != Null) {
								auto memory = AllocateStack();
								
								// Store board title
								if(GetTextLength(*GetTitleBar()) > 0)
									Push(GetText(*GetTitleBar()), false, memory);
								Push(Null, true, memory);
								
								// Notes
								BeginNotesLoop(n) {
									if(NoteMemoryIsInUse(n) == true) {
										// Store the pos
										f32* f = PushType(f32, memory);
										*f = n->text.container.pos.x;
										f = PushType(f32, memory);
										*f = n->text.container.pos.y;
										
										// Text contents
										if(NoteHasTitle(n) == true)
											Push(GetText(n->title), false, memory);
										Push(Null, true, memory);
										
										if(GetTextLength(n->text) > 0)
											Push(GetText(n->text), false, memory);
										Push(Null, true, memory);
									}
								}
								EndNotesLoop();
							
								SaveFile(memory.memory, memory.size, path);
								Win32DisplayInfoBox("File saved!", false);
								
								Free(memory);
							}
						}
						else if(it == 1) { // Load
							// Set correct directory
							if(AreEqual(Win32GetCurrentDirectory(), "Boards") == false) {
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
								MovePtr(data, GetLength(boardTitle) + 1);
								if(boardTitle[0] != '\0') {
									ClearText(*GetTitleBar());
									Insert(boardTitle, GetLength(boardTitle), *GetTitleBar(), 0);
								}
								
								Clear(state.notes.memory.memory, state.notes.memory.size);
								
								// Extract notes
								while(data < (ui8*)file.memory + file.size) {
									f32 x = ReadMemMovePtr(data, f32);
									f32 y = ReadMemMovePtr(data, f32);
									
									char* title = (char*)data;
									MovePtr(data, GetLength(title) + 1);
									
									char* text = (char*)data;
									MovePtr(data, GetLength(text) + 1);
									
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
		if(osState.mouseLeftDoubleClick == true && MouseOverlapsGUI(GetTitleBar()->container) == true) {
			auto* tb = GetTitleBar();
			
			if(TextIsBeingUpdated() == true && TitleIsBeingUpdated() == false)
				EndTextUpdate();
			SetCurrentNote(Null);
			BeginTextUpdate(*tb);
			
			SetCursorPos(UnpackVector(state.mouse.pos - GetTextRectangle(*tb).pos));
			
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
		else if(NoteTextIsBeingUpdated() == true && MouseLeftDownThisFrame() == true && MouseOverlapsGUI(state.toolBar.buttons[1].background) == true) { // Add a bullet point
			InsertCharAtCursor(BulletPointChar); // Will check viability first	
			goto label_rendering;
		}
		else if(GetCurrentNote() != Null && MouseLeftDownThisFrame() == true && MouseOverlapsGUI(state.toolBar.buttons[2].background) == true) { // Add a title to the currently selected note
			auto* n = GetCurrentNote();
			if(NoteHasTitle(n) == false) {
				auto textRec = GetTextRectangle(n->text);
				n->title = AllocateTextBody(textRec.left, GetTopRight(textRec).y, NoteMinWidth, NoteTextBorder, NoteTitleTextHeight, TextBodyFlagLetters);
				Insert("Title", GetLength("Title"), n->title, 0);
				UpdateNoteContainers(n);
			}
			goto label_rendering;
		}
		else if(MouseLeftDownThisFrame() == true && MouseOverlapsGUI(state.toolBar.buttons[3].background) == true) { // Toggle colour wheel
			auto* panel = GetColourPanel();
			Toggle(panel->display);
			if(panel->display == true) {
				panel->frame.left = GetTopRight(GetToolBar()->background).x + 100;
				panel->frame.height = ColourPanelHeight;
				panel->frame.bottom = GetCenter(GetToolBar()->buttons[3].background).y - panel->frame.height / 2;
				
				auto wheel = GetColourPanelWheelRectangle();
				panel->frame.width = wheel.width + ColourPanelSliderWidth + ColourPanelRGBBoxWidth + ColourPanelEdgeOffset * 4;
				
				panel->selection = GetCenter(GetColourPanelWheelRectangle());
				panel->sliderCenterY = GetTopRight(GetColourPanelWheelRectangle()).y;
				
				f32 left = panel->frame.left + panel->frame.width - ColourPanelEdgeOffset - ColourPanelRGBBoxWidth;
				f32 height = ColourPanelRGBBoxTextHeight + ColourPanelRGBBoxOffset * 2;
				f32 offset = (wheel.height - height * 4) / 3;
				panel->red = AllocateTextBody(left, wheel.bottom + wheel.height - height, ColourPanelRGBBoxWidth, ColourPanelRGBBoxOffset, ColourPanelRGBBoxTextHeight, Null);
				panel->green = AllocateTextBody(left, panel->red.container.bottom - offset - height, ColourPanelRGBBoxWidth, ColourPanelRGBBoxOffset, ColourPanelRGBBoxTextHeight, Null);
				panel->blue = AllocateTextBody(left, panel->green.container.bottom - offset - height, ColourPanelRGBBoxWidth, ColourPanelRGBBoxOffset, ColourPanelRGBBoxTextHeight, Null);
				panel->hex = AllocateTextBody(left, wheel.bottom, ColourPanelRGBBoxWidth, ColourPanelRGBBoxOffset, ColourPanelRGBBoxTextHeight, Null);
			}
			else {
				FreeText(panel->red);
				FreeText(panel->green);
				FreeText(panel->blue);
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
				if(NoteMemoryIsInUse(n) == true && MouseOverlapsCanvas(GetNoteOverallRectangle(n)) == true) {
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
				auto* n = GetCurrentNote();
				
				auto mousePosCanvas = ConvertToCanvasSpace(state.mouse.pos);
				if(NoteHasTitle(n) == true && MouseOverlapsCanvas(n->title.container) == true) { // Update title
					BeginTextUpdate(GetCurrentNote()->title);
					
					// Position mouse cursor more precisely
					f32 x = mousePosCanvas.x - GetTextRectangle(n->title).left;
					SetCursorPos(x, 0);
				}
				else if(MouseOverlapsCanvas(n->text.container) == true) { // Update text
					auto* n = GetCurrentNote();
					
					BeginTextUpdate(n->text);
					
					vector pos = mousePosCanvas - GetTextRectangle(n->text).pos;
					SetCursorPos(pos.x, pos.y);
				}
			}
		}
		else if(MouseLeftDownThisFrame() == true) { // Select
			auto* previouslySelected = GetCurrentNote();
			SetCurrentNote(Null);
		
			auto mousePosCanvas = ConvertToCanvasSpace(state.mouse.pos.x, state.mouse.pos.y);
			BeginNotesLoop(n) {
				if(NoteMemoryIsInUse(n) == true && MouseOverlapsCanvas(GetNoteOverallRectangle(n)) == true) {
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
			auto* n = GetCurrentNote();
			Assert(n != Null);
			
			// Mouse moves in viewport space
			vector newPosCanvas = { n->text.container.pos.x + (f32)state.mouse.translation.x / state.canvas.scale, 
														  n->text.container.pos.y + (f32)state.mouse.translation.y / state.canvas.scale };
														 
			// When moving a new note outside of the toolbar, ensure it can't be moved back in
			if(state.notes.justCreated == true) {
				f32 toolbarEdgeCanvas = ConvertToCanvasSpace(state.toolBar.background.left + state.toolBar.background.width, Null).x;
				if(newPosCanvas.x >= toolbarEdgeCanvas)
					state.notes.justCreated = false;
			}
			
			n->text.container.pos = newPosCanvas;
			if(NoteHasTitle(n) == true) {
				n->title.container.left = n->text.container.left;
				n->title.container.bottom = GetTopRight(n->text.container).y;
			}
		}
		else if(state.notes.moving == true && state.mouse.leftDown == false) { // Drop
			state.notes.moving = false;
			
			// If the note was just created, drop it outside of the toolbar
			Assert(state.notes.selected != Null);
			f32 toolbarEdgeCanvas = ConvertToCanvasSpace(state.toolBar.background.left + state.toolBar.background.width, Null).x;
			if(GetCurrentNote()->text.container.pos.x < toolbarEdgeCanvas && state.notes.justCreated == true)
				GetCurrentNote()->text.container.pos.x = toolbarEdgeCanvas;
			
			state.notes.justCreated = false;
		}
		else if(state.notes.selected != Null && TextIsBeingUpdated() == false && (osState.deletePressed == true || osState.backspacePressed == true)) { // Delete note
			FreeText(GetCurrentNote()->text);
			if(IsValid(GetCurrentNote()->title) == true)
				FreeText(GetCurrentNote()->title);
			Clear(state.notes.selected, sizeof(note));
			SetCurrentNote(Null);
		}
		
		// Update text somewhere
		if(TextIsBeingUpdated() == true) {
			auto pipelineUpdate = RunTextUpdatePipeline(osState);
			if(NoteTextIsBeingUpdated() == true)
				UpdateNoteContainers(GetCurrentNote());
			
			if(osState.escapePressed == true && NoteTextIsBeingUpdated() == true)
				SetCurrentNote(Null);
			else if(osState.mouseLeftClickDown == true && MouseIsWithinToolbar() == false) { // Left mouse click outside of the tool bar, check where and decide
				Assert(TitleIsBeingUpdated() == true || GetCurrentNote() != Null);
				if(TitleIsBeingUpdated() == true && MouseOverlapsGUI(GetTitleBar()->container) == false ||
				   MouseOverlapsCanvas(GetNoteOverallRectangle(GetCurrentNote())) == false)
					EndTextUpdate();
			}
			else if( // If we're updating a note title and want to move down, go to the text section
							pipelineUpdate.wantToLeaveTextBodyDown == true && NoteHasTitle(GetCurrentNote()) == true && GetCurrentTextBody() == &GetCurrentNote()->title) 
			{				
				auto* n = GetCurrentNote();
				
				f32 cursorXAbs = GetTextRectangle(n->title).left + GetCursorPos().x;
				
				EndTextUpdate();
				BeginTextUpdate(n->text);
				
				auto textRec = GetTextRectangle(n->text);
				f32 xRel = cursorXAbs - textRec.left;
				f32 yRel = textRec.height - n->text.textHeight;
				SetCursorPos(xRel, yRel);
			}
			else if( // If we're updating a note title and want to move down, go to the text section
							pipelineUpdate.wantToLeaveTextBodyUp == true && NoteTextIsBeingUpdated() == true && NoteHasTitle(GetCurrentNote()) == true && GetCurrentTextBody() == &GetCurrentNote()->text)
			{
				auto* n = GetCurrentNote();
				
				f32 cursorXAbs = GetTextRectangle(n->text).left + GetCursorPos().x;
				
				EndTextUpdate();
				BeginTextUpdate(n->title);
				
				f32 xRel = cursorXAbs - GetTextRectangle(n->title).left;
				f32 yRel = GetTextRectangle(n->text).height;
				SetCursorPos(xRel, yRel);
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
					DrawRectangleFull(UnpackRectangle(GetNoteOverallRectangle(n)), 255, 255, 255);
			}
		}
		EndNotesMemoryLoop();

		// Draw border on a selected note
		if(state.notes.selected != Null && state.notes.justCreated == false)
			DrawRectangleBorder(UnpackRectangle(GetNoteOverallRectangle(GetCurrentNote())), UIBorderThickness, 0, 0, 0);

		// @TODO - Is Win32GetMousePoswidthinClient() needed anymore?

		// Render notes text and title if it has one, then update note background rectangle
		BeginNotesLoop(n) {
			if(NoteMemoryIsInUse(n) == true) {
				if(GetTextLength(n->text) > 0)
					Render(n->text);
				if(NoteHasTitle(n) == true && GetTextLength(n->title) > 0)
					Render(n->title);
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
				RenderText((char*)b->text, GetLength(b->text), GetCenter(b->background).x, b->textBottom, tb->textHeight, true);
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
			auto* tb = GetTitleBar();
			DrawRectangleFull(UnpackRectangle(tb->container), 255, 255, 255);
			if(GetTextLength(*tb) > 1)
				Render(*tb);
			
			// Draw separator
			glLineWidth(3);
			glColor3f(0, 0, 0);
			glBegin(GL_LINES);
			glVertex2s(tb->container.left, tb->container.bottom);
			glVertex2s(tb->container.left + tb->container.width, tb->container.bottom);
			glEnd();
			AssertOpenGL();
		}
			
		// If a note was just created, draw in front of the tool bar
		if(state.notes.selected != Null && state.notes.justCreated == true) {
			SetCanvasProjetionMatrix();
			auto rec = GetNoteOverallRectangle(GetCurrentNote());
			DrawRectangleFull(UnpackRectangle(rec), 255, 255, 255);
			DrawRectangleBorder(UnpackRectangle(rec), UIBorderThickness, 0, 0, 0);
		}
		
		// Draw cursor if needed
		if(TextIsBeingUpdated() == true) {
			SetGUIProjectionMatrix();
			if(TitleIsBeingUpdated() == false)
				SetCanvasProjetionMatrix();
			
			f32 alpha = GetCursorAlphaValue();
			vector cursorPos = {};
			if(TitleIsBeingUpdated() == true)
				cursorPos = GetTextRectangle(*GetTitleBar()).pos + GetCursorPos();
			else if (NoteTextIsBeingUpdated() == true)
				cursorPos = GetTextRectangle(GetCurrentNote()->text).pos + GetCursorPos();
			else { // Note title
				auto* n = GetCurrentNote();
				Assert(n != Null);
				Assert(NoteHasTitle(n) == true);
				Assert(GetCurrentTextBody() == &n->title);
				cursorPos = GetTextRectangle(GetCurrentNote()->title).pos + GetCursorPos();
			}
			
			glLineWidth(2);
			glColor4f(0, 0, 0, alpha);
			glBegin(GL_LINES);
			glVertex2f(cursorPos.x, cursorPos.y);
			glVertex2f(cursorPos.x, cursorPos.y + GetCurrentTextBody()->textHeight);
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
						break;
					}
				}
			}
			
			ForAll(GetArrayLength(m->buttons)) {
				auto* b = m->buttons + it;
				RenderText((char*)b->text, GetLength(b->text), b->left + TopMenuButtonWidth / 2, GetCenter(m->background).y - TopMenuTextHeight / 2, TopMenuTextHeight, true);
				
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
			DrawRectangleBorder(UnpackRectangle(panel->frame), UIBorderThickness, 0, 0, 0);
			
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
				DrawCircleBorder(GetCenter(r).x, GetCenter(r).y, r.width / 2, UIBorderThickness, 0, 0, 0);
				
				// Draw selection
				DrawCircleBorder(UnpackVector(panel->selection), 10, UIBorderThickness, 0, 0, 0);
				
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
				DrawRectangleBorder(rec.left, rec.bottom, rec.width, rec.height, UIBorderThickness, 0, 0, 0);
				
				// Draw selection
				DrawRectangleBorder(rec.left - 3, panel->sliderCenterY - 5, rec.width + 6, 10, UIBorderThickness, 0, 0, 0);
			}
			
			// Sample colour and RGB text bodies
			{
				auto* panel = GetColourPanel();
				
				// Final colour
				f32 finalRed = 255;
				f32 finalGreen = 255;
				f32 finalBlue = 255;
				{
					auto rec = GetColourPanelSliderRectangle();
					f32 sliderScale = (panel->sliderCenterY - rec.bottom) / rec.height;
					finalRed = wheelRed * sliderScale;
					finalGreen = wheelGreen * sliderScale;
					finalBlue = wheelBlue * sliderScale;
					
					f32 centerX = panel->frame.left + panel->frame.width / 8;
					f32 centerY = panel->frame.bottom + panel->frame.height / 4;
					f32 radius = panel->frame.width / 5 / 2;
					
					//  Colour circle
					glBegin(GL_TRIANGLE_FAN);
					glColor3f(finalRed, finalGreen, finalBlue);
					ui8 vertices = 36;
					FromToInc(0, vertices + 1) {
						f32 angle = it * 360 / vertices;
						f32 x = centerX - Sine(angle) * radius;
						f32 y = centerY + Cos(angle) * radius;
						glVertex2f(x, y);
					}
					glEnd();
					
					// Frame
					DrawCircleBorder(centerX, centerY, radius, UIBorderThickness, 0, 0, 0);
				}
				
				// RGB panels
				DrawRectangleBorder(UnpackRectangle(panel->red.container), UIBorderThickness, 0, 0, 0);
				DrawRectangleBorder(UnpackRectangle(panel->green.container), UIBorderThickness, 0, 0, 0);
				DrawRectangleBorder(UnpackRectangle(panel->blue.container), UIBorderThickness, 0, 0, 0);
				DrawRectangleBorder(UnpackRectangle(panel->hex.container), UIBorderThickness, 0, 0, 0);
					
				ForAll(3) {
					// Box
					f32 middleX = panel->frame.left + (panel->frame.width / 4) * (it + 1) + panel->frame.width / 8;
					f32 middleY = panel->frame.bottom + panel->frame.height / 4;
					f32 height = panel->frame.height / 5;
					
					// Text fields
					if(it == 0) {
						ClearText(panel->red);
						char* string = ToString((ui8)(finalRed * 255));
						if(GetLength(string) == 1)
							string = Concatenate(2, "00", string);
						else if(GetLength(string) == 2)
							string = Concatenate(2, "0", string);
						Insert(string, 3, panel->red, 0);
						Render(panel->red);
						Free(string);
					}
					else if(it == 1) {
						ClearText(panel->green);
						char* string = ToString((ui8)(finalGreen * 255));
						if(GetLength(string) == 1)
							string = Concatenate(2, "00", string);
						else if(GetLength(string) == 2)
							string = Concatenate(2, "0", string);
						Insert(string, 3, panel->green, 0);
						Render(panel->green);
						Free(string);
					}
					else {
						ClearText(panel->blue);
						char* string = ToString((ui8)(finalBlue * 255));
						if(GetLength(string) == 1)
							string = Concatenate(2, "00", string);
						else if(GetLength(string) == 2)
							string = Concatenate(2, "0", string);
						Insert(string, 3, panel->blue, 0);
						Render(panel->blue);
						Free(string);
					}	
				}
				
				// Display RGB as a single number in hexadecimal
				{
					ClearText(panel->hex);
					ui8  r = finalRed * 255;
					ui8  g = finalGreen * 255;
					ui8  b = finalBlue * 255;
					ui32 h = r << 16 | g << 8 | r; // @WIP
				}
			}
		}
		
		// Store state before next frame
		state.mouse.lastLeftDown = state.mouse.leftDown;

		Win32EndGUIUpdateLoop();
	}
	
	return 0;
}