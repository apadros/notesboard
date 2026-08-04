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

#include <stdio.h> // For conversion to hex

GUIAppEntryPoint(instance) {
	Win32InitGUI("Bola Pad v0.0", instance);
	
	// Init top menu
	{
		auto* m = GetTopMenu();
		m->background.left = 0;
		m->background.width = Win32GetProgramWindowClientSize().x;
		m->background.height = TopMenuHeight;
		m->background.bottom = Win32GetProgramWindowClientSize().y - m->background.height;
		m->save = AllocateButton(0, m->background.bottom, TopMenuButtonWidth, TopMenuHeight, "Save", TopMenuTextHeight, 0, 0, 0, 1);
		m->load = AllocateButton(TopMenuButtonWidth, m->background.bottom, TopMenuButtonWidth, TopMenuHeight, "Load", TopMenuTextHeight, 0, 0, 0, 1);
	}

	// Init title bar
	state.titleBar = AllocateTextBody(0, GetTopMenu()->background.bottom - TitleBarHeight, Win32GetProgramWindowClientSize().width, (TitleBarHeight - TitleBarTextHeight) / 2, TitleBarTextHeight, TextBodyFlagLetters);
	Insert("Title", GetLength("Title"), state.titleBar, 0);

	// Init toolbar
	{
		auto* tb = &state.toolBar;
		tb->background.left = 0;
		tb->background.bottom = 0;
		tb->background.width = ToolbarWidth;
		tb->background.height = GetTopMenu()->background.bottom - GetTitleBar()->container.height;

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
		
		// Run text update pipeline first
		text_update_pipeline_data textUpdatePipelineData = {};
		if(TextIsBeingUpdated() == true)
			textUpdatePipelineData = RunTextUpdatePipeline(osState);
		
		// @SECTION - Colour panel
		// Update colour panel first, nothing else can be interacted with while it is running
		if(GetColourPanel()->display == true) {
			auto* panel = GetColourPanel();
			
			// If we're updating the any of the rgb/hex fields and click anywhere else, end text update
			if(TextIsBeingUpdated() == true && Win32MouseLeftDownThisFrame(osState) == true) {
				text_body* bodies[] = { &panel->red, &panel->green, &panel->blue, &panel->hex };
				ForAll(GetArrayLength(bodies)) {
					if(GetCurrentTextBody() == bodies[it] && MouseOverlapsGUI(osState, bodies[it]->container) == false) {
						EndTextUpdate();
						break;
					}
				}
			}
						
			if(ButtonClicked(panel->ok, osState) == true || ButtonClicked(panel->cancel, osState) == true) { // If OK or Cancel are clicked
				// Store currently selected colour
				if(ButtonClicked(panel->ok, osState) == true) {
					panel->savedCurrentColour = panel->currentColour;
					if(panel->colourBeingUpdated != Null)
						*panel->colourBeingUpdated = panel->currentColour;
				}
				else if(panel->colourBeingUpdated != Null)
					*panel->colourBeingUpdated = panel->savedCurrentColour;
				
				if(TextIsBeingUpdated() == true && GetCurrentTextBody() == &panel->hex)
					EndTextUpdate();
				
				FreeText(panel->red);
				FreeText(panel->green);
				FreeText(panel->blue);
				FreeText(panel->hex);
				FreeButtonText(panel->ok);
				FreeButtonText(panel->cancel);
				FreeButtonText(panel->save);
				panel->colourBeingUpdated = Null;
				
				panel->display = false;
			}
			else if(ButtonClicked(panel->save, osState) == true) { // Save clicked
				if(panel->favouriteSelected != Null) // Store in currently selected favourite
					panel->favourites[panel->favouriteSelected - 1] = panel->currentColour;
				else { // Grab next one available
					ForAll(GetArrayLength(panel->favourites)) {
						auto* f = panel->favourites + it;
						if(ColourPanelColourIsInited(*f) == false) {
							*f = panel->currentColour;
							break;
						}
					}
				}
			}
			else if(osState.mouseLeftDoubleClick == true && MouseOverlapsGUI(osState, panel->red.container) == true) // Interact with red field
				BeginTextUpdate(panel->red);
			else if(osState.mouseLeftDoubleClick == true && MouseOverlapsGUI(osState, panel->green.container) == true) // Interact with green field
				BeginTextUpdate(panel->green);
			else if(osState.mouseLeftDoubleClick == true && MouseOverlapsGUI(osState, panel->blue.container) == true) // Interact with blue field
				BeginTextUpdate(panel->blue);
			else if(osState.mouseLeftDoubleClick == true && MouseOverlapsGUI(osState, panel->hex.container) == true) // Interact with hex field
				BeginTextUpdate(panel->hex);
			else if(Win32MouseLeftDownThisFrame(osState) == true && Overlap(UnpackVector(GetCenter(GetColourPanelWheelRectangle())), UnpackVector(osState.mousePos), GetColourPanelWheelRectangle().width / 2) == true) { // Begin colour wheel udpate
				panel->updatingCurrentColour = true;
				panel->currentColour.wheelSelection = osState.mousePos;
				if(TextIsBeingUpdated() == true && GetCurrentTextBody() == &panel->hex)
					EndTextUpdate();
			}
			else if(panel->updatingCurrentColour == true) { // Update current colour
				if(osState.mouseLeftDown == true) {
					auto newPos = panel->currentColour.wheelSelection + osState.mouseTranslation;
					auto wheel = GetColourPanelWheelRectangle();
					if(Overlap(UnpackVector(GetCenter(wheel)), UnpackVector(newPos), wheel.width / 2) == true) // Check if new position lies within the wheel
						panel->currentColour.wheelSelection += osState.mouseTranslation;
					if(panel->colourBeingUpdated != Null)
						*panel->colourBeingUpdated = panel->currentColour;
				}
				else
					panel->updatingCurrentColour = false;
			}
			else if(Win32MouseLeftDownThisFrame(osState) == true && MouseOverlapsGUI(osState, GetColourPanelSliderRectangle()) == true) { // Begin colour slider update
				panel->updatingSlider = true;
				panel->currentColour.sliderCenterY = osState.mousePos.y;
			}
			else if(panel->updatingSlider == true) { // Update slider position
				if(osState.mouseLeftDown == true) {
					auto newPos = panel->currentColour.wheelSelection + osState.mouseTranslation;
					auto rec = GetColourPanelSliderRectangle();
					if(MouseOverlapsGUI(osState, GetColourPanelSliderRectangle()) == true) // Check if new position lies within the wheel
						panel->currentColour.sliderCenterY += osState.mouseTranslation.y;
					if(panel->colourBeingUpdated != Null)
						*panel->colourBeingUpdated = panel->currentColour;
				}
				else
					panel->updatingSlider = false;
			}
			else if(Win32MouseLeftDownThisFrame(osState) == true) { // Check for selection of favourite colours
				f32 start = panel->frame.left + ColourPanelEdgeOffset;
				f32 end = panel->save.rectangle.left - ColourPanelEdgeOffset;
				ui8 count = GetArrayLength(panel->favourites) + 1;
				auto* layouts = GetUIElementLayouts(start, end, count, ColourPanelFavouritesLayerHeight,
																						ColourPanelFavouritesLayerHeight / 2, ColourPanelFavouritesLayerHeight / 2, ColourPanelFavouritesLayerHeight / 2,
																						ColourPanelFavouritesLayerHeight / 2, ColourPanelFavouritesLayerHeight / 2, ColourPanelFavouritesLayerHeight / 2);
				f32 centerY = GetColourPanelWheelRectangle().bottom - ColourPanelEdgeOffset - ColourPanelFavouritesLayerHeight / 2;
				FromTo(1, count) {
					auto* l = layouts + it;
					if(Overlap(UnpackVector(osState.mousePos), l->center, centerY, l->size / 2) == true) {
						panel->favouriteSelected = it;
						panel->currentColour = panel->favourites[it - 1];
						if(ColourPanelColourIsInited(panel->currentColour) == false) {
							panel->currentColour.wheelSelection = GetCenter(GetColourPanelWheelRectangle());
							panel->currentColour.sliderCenterY = GetTopRight(GetColourPanelSliderRectangle()).y;
						}
						UpdateColourPanelRGBHexText(panel->currentColour);
						if(panel->colourBeingUpdated != Null)
							*panel->colourBeingUpdated = panel->currentColour;
						break;
					}
				}
				FreeUIElementLayouts(layouts);
			}
			else if(IsBeingUpdated(panel->red) == true) { // Update current colour based on updates to red text body
				if(osState.escPressed == true) { // Return to what was there before
					EndTextUpdate()
					UpdateColourPanelRGBHexText(panel->currentColour);
				}
				else if(osState.enterPressed == true) { // Accept new changes
					EndTextUpdate();
					
					// For now just update the hex field and final colour without updating the colour panel colour mechanics
					char* text = GetText(panel->red);
					ui8   i = StringToInt(text, Null);
					
					// @TODO - Update current colour
				}
			}
			
			goto label_rendering; // Need this since it will partially overlap the canvas
		}
		
		// @SECTION - Top menu
		if(MouseOverlapsGUI(osState, GetTopMenu()->background) == true) {
			auto* m = GetTopMenu();
			if(ButtonClicked(m->save, osState) == true) {
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
					
					// Store custom colours
					// For now just store them all, including uninitialised ones
					ui8 colours = GetArrayLength(GetColourPanel()->favourites);
					PushInstance(colours, memory);
					ForAll(GetArrayLength(GetColourPanel()->favourites)) {
						auto* f = GetColourPanel()->favourites + it;
						Push(f, sizeof(*f), memory);
					}

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
			else if(ButtonClicked(m->load, osState) == true) {
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

					// Extract board title
					char* boardTitle = (char*)data;
					MovePtr(data, GetLength(boardTitle) + 1);
					if(boardTitle[0] != '\0') {
						ClearText(*GetTitleBar());
						Insert(boardTitle, GetLength(boardTitle), *GetTitleBar(), 0);
					}
					
					// Extract custom colours
					ui8 count = ReadMemMovePtr(data, ui8);
					Assert(count <= GetArrayLength(GetColourPanel()->favourites));
					ForAll(count) {
						auto* f = GetColourPanel()->favourites + it;
						Copy(data, sizeof(*f), f);
						MovePtr(data, sizeof(*f));
					}

					// Extract notes
					Clear(state.notes.memory.memory, state.notes.memory.size);
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

		// @SECTION - Title bar
		if(osState.mouseLeftDoubleClick == true && MouseOverlapsGUI(osState, GetTitleBar()->container) == true) {
			auto* tb = GetTitleBar();

			if(TextIsBeingUpdated() == true && TitleIsBeingUpdated() == false)
				EndTextUpdate();
			
			if(TitleIsBeingUpdated() == false)
				BeginTextUpdate(*tb);

			SetCursorPos(UnpackVector(osState.mousePos - GetTextRectangle(*tb).pos));
			
			goto label_rendering;
		}
		else if( // Clicking out of the title bar while updating it
						TitleIsBeingUpdated() == true && Win32MouseLeftDownThisFrame(osState) == true && MouseOverlapsGUI(osState, GetTitleBar()->container) == false)
			EndTextUpdate();

		// @SECTION - Toolbar
		if(Win32MouseLeftDownThisFrame(osState) == true && MouseOverlapsGUI(osState, state.toolBar.buttons[0].background) == true) { // Create new note
			auto pos = ConvertToCanvasSpace(0, osState.mousePos.y - NoteMinHeight / 2);
			auto* n = CreateNote(pos, Null, Null);
			state.notes.selected = n;
			state.notes.justCreated = true;
			state.notes.moving = true;
			goto label_rendering;
		}
		else if(NoteTextIsBeingUpdated() == true && Win32MouseLeftDownThisFrame(osState) == true && MouseOverlapsGUI(osState, state.toolBar.buttons[1].background) == true) // Add a bullet point
			InsertCharAtCursor(BulletPointChar); // Will check viability first
		else if(GetCurrentNote() != Null && Win32MouseLeftDownThisFrame(osState) == true && MouseOverlapsGUI(osState, state.toolBar.buttons[2].background) == true) { // Add a title to the currently selected note
			auto* n = GetCurrentNote();
			if(NoteHasTitle(n) == false) {
				auto textRec = GetTextRectangle(n->text);
				n->title = AllocateTextBody(textRec.left, GetTopRight(textRec).y, NoteMinWidth, NoteTextBorder, NoteTitleTextHeight, TextBodyFlagLetters);
				Insert("Title", GetLength("Title"), n->title, 0);
				UpdateNoteContainers(n);
			}
		}
		else if(GetColourPanel()->display == false && Win32MouseLeftDownThisFrame(osState) == true && MouseOverlapsGUI(osState, state.toolBar.buttons[3].background) == true) // Open colour panel
			OpenColourPanel(Null);

		// @SECTION - Notes
		if(osState.mouseLeftDoubleClick == true) { // Begin writing regardless of whether a note is selected @TODO - Technically a note would have already been selected be 1st mouse click, simplify?
			auto* previouslySelected = GetCurrentNote();
			SetCurrentNote(Null);

			BeginNotesLoop(n) {
				if(NoteMemoryIsInUse(n) == true && MouseOverlapsCanvas(osState, GetNoteOverallRectangle(n)) == true) {
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

				auto mousePosCanvas = ConvertToCanvasSpace(osState.mousePos);
				if(NoteHasTitle(n) == true && MouseOverlapsCanvas(osState, n->title.container) == true) { // Update title
					BeginTextUpdate(GetCurrentNote()->title);

					// Position mouse cursor more precisely
					f32 x = mousePosCanvas.x - GetTextRectangle(n->title).left;
					SetCursorPos(x, 0);
					
					goto label_rendering;
				}
				else if(MouseOverlapsCanvas(osState, n->text.container) == true) { // Update text
					auto* n = GetCurrentNote();

					BeginTextUpdate(n->text);

					vector pos = mousePosCanvas - GetTextRectangle(n->text).pos;
					SetCursorPos(pos.x, pos.y);
					
					goto label_rendering;
				}
			}
		}
		else if(Win32MouseLeftDownThisFrame(osState) == true && NoteIsBeingUpdated() == false) { // Select only when not updating text
			auto* previouslySelected = GetCurrentNote();
			SetCurrentNote(Null);

			auto mousePosCanvas = ConvertToCanvasSpace(osState.mousePos.x, osState.mousePos.y);
			BeginNotesLoop(n) {
				if(NoteMemoryIsInUse(n) == true && MouseOverlapsCanvas(osState, GetNoteOverallRectangle(n)) == true) {
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
		else if(state.notes.moving == true && osState.mouseLeftDown == true) { // Move
			auto* n = GetCurrentNote();
			Assert(n != Null);

			// Mouse moves in viewport space
			vector newPosCanvas = { n->text.container.pos.x + (f32)osState.mouseTranslation.x / state.canvas.scale,
														  n->text.container.pos.y + (f32)osState.mouseTranslation.y / state.canvas.scale };

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
		else if(state.notes.moving == true && osState.mouseLeftDown == false) { // Drop
			state.notes.moving = false;

			// If the note was just created, drop it outside of the toolbar
			Assert(state.notes.selected != Null);
			f32 toolbarEdgeCanvas = ConvertToCanvasSpace(state.toolBar.background.left + state.toolBar.background.width, Null).x;
			if(GetCurrentNote()->text.container.pos.x < toolbarEdgeCanvas && state.notes.justCreated == true)
				GetCurrentNote()->text.container.pos.x = toolbarEdgeCanvas;

			state.notes.justCreated = false;
		}
		else if(GetCurrentNote() != Null && TextIsBeingUpdated() == false && (osState.deletePressed == true || osState.backspacePressed == true)) { // Delete note
			FreeText(GetCurrentNote()->text);
			if(IsValid(GetCurrentNote()->title) == true)
				FreeText(GetCurrentNote()->title);
			Clear(state.notes.selected, sizeof(note));
			SetCurrentNote(Null);
		}
		else if(NoteTitleIsBeingUpdated() == true && textUpdatePipelineData.wantToLeaveTextBodyDown == true) { // If we're updating a note title and want to move down, go to the text section
			auto* n = GetCurrentNote();

			f32 cursorXAbs = GetTextRectangle(n->title).left + GetCursorPos().x;

			EndTextUpdate();
			BeginTextUpdate(n->text);

			auto textRec = GetTextRectangle(n->text);
			f32 xRel = cursorXAbs - textRec.left;
			f32 yRel = textRec.height - n->text.textHeight;
			SetCursorPos(xRel, yRel);
		}
		else if(NoteTextIsBeingUpdated() == true && textUpdatePipelineData.wantToLeaveTextBodyUp == true && NoteHasTitle(GetCurrentNote()) == true) { // If we're updating a note title and want to move down, go to the text section
			auto* n = GetCurrentNote();

			f32 cursorXAbs = GetTextRectangle(n->text).left + GetCursorPos().x;

			EndTextUpdate();
			BeginTextUpdate(n->title);

			f32 xRel = cursorXAbs - GetTextRectangle(n->title).left;
			f32 yRel = GetTextRectangle(n->text).height;
			SetCursorPos(xRel, yRel);
		}
		else if(GetCurrentNote() != Null && Win32MouseLeftDownThisFrame(osState) == true && MouseOverlapsCanvas(osState, GetNoteOverallRectangle(GetCurrentNote())) == false) { // Clicking out of the currently selected note
			if(NoteIsBeingUpdated() == true)
				EndTextUpdate();
			SetCurrentNote(Null);
		}
		if(NoteTextIsBeingUpdated() == true || NoteTitleIsBeingUpdated() == true)
			UpdateNoteContainers(GetCurrentNote());
		
		// If double clicking hasn't done anything else, allow it to update the canvas's background colour
		if(osState.mouseLeftDoubleClick == true && osState.mousePos.x > GetTopRight(GetToolBar()->background).width && osState.mousePos.y < GetTitleBar()->container.bottom)
			OpenColourPanel(&state.canvas.colour);
		
		// @SECTION - Update scaling
		if(osState.mouseWheelRotation != 0.0f) {
			auto mousePosPre = ConvertToCanvasSpace(osState.mousePos);
			state.canvas.scale += osState.mouseWheelRotation / 10;
			if(state.canvas.scale <= 0.0f)
				state.canvas.scale = 0.1f;
			auto   mousePosPost = ConvertToCanvasSpace(osState.mousePos);
			vector mouseTranslationCanvas = mousePosPost - mousePosPre;
			state.canvas.translation += mouseTranslationCanvas * state.canvas.scale;
		}

		// @SECTION - Update translations
		if(osState.mouseRightDown == true && (osState.mouseTranslation.x != 0 || osState.mouseTranslation.y != 0)) {
			state.canvas.translation.x += osState.mouseTranslation.x;
			state.canvas.translation.y += osState.mouseTranslation.y;
		}

		// Use gotos to keep things tidy instead of if else everywhere
		label_rendering:
		
		// Render canvas background
		SetGUIProjectionMatrix();
		{
			colour c = ConvertColourPanelColourToRGB(state.canvas.colour);
			DrawRectangleFull(0, 0, canvas.width, canvas.height, UnpackColourUI8(c), 1);
		}

		SetCanvasProjetionMatrix();
		
		// @SECTION - Render notes
		BeginNotesMemoryLoop(n) {
			if(NoteMemoryIsInUse(n) == true) {
				bool draw = true;
				if(state.notes.selected != Null && state.notes.selected == n && state.notes.justCreated == true) // Recently created notes will be drawn in front of the UI, further down
					draw = false;

				if(draw == true) {
					DrawRectangleFull(UnpackRectangle(GetNoteOverallRectangle(n)), 255, 255, 255, 1);
					DrawRectangleBorder(UnpackRectangle(GetNoteOverallRectangle(n)), UIBorderThickness, 230, 230, 230);
				}
			}
		}
		EndNotesMemoryLoop();
		if(GetCurrentNote() != Null && state.notes.justCreated == false) // Draw border on a selected note
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

		// @SECTION - Render the toolbar
		{
			auto* tb = &state.toolBar;
			DrawRectangleFull(UnpackRectangle(state.toolBar.background), 255, 255, 255, 1); // Background

			ForAll(GetArrayLength(tb->buttons)) { // Buttons
				auto* b = tb->buttons + it;
				DrawRectangleFull(UnpackRectangle(b->background), 255, 0, 0, 1);
				RenderText((char*)b->text, GetLength(b->text), GetCenter(b->background).x, b->textBottom, tb->textHeight, true);
			}

			// Draw separator
			glLineWidth(UIBorderThickness);
			glColor3f(0, 0, 0);
			glBegin(GL_LINES);
			glVertex2s(tb->background.width, 0);
			glVertex2s(tb->background.width, tb->background.height);
			glEnd();
			AssertOpenGL();
		}

		// @SECTION - Render the ritle bar
		{
			auto* tb = GetTitleBar();
			DrawRectangleFull(UnpackRectangle(tb->container), 255, 255, 255, 1);
			if(GetTextLength(*tb) > 1)
				Render(*tb);

			// Draw separator
			glLineWidth(UIBorderThickness);
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
			DrawRectangleFull(UnpackRectangle(rec), 255, 255, 255, 1);
			DrawRectangleBorder(UnpackRectangle(rec), UIBorderThickness, 0, 0, 0);
		}

		// @SECTION - Render top menu
		{
			SetGUIProjectionMatrix();

			auto* m = GetTopMenu();
			DrawRectangleFull(UnpackRectangle(m->background), 146, 139, 183, 1);

			// Render text
			Render(m->save, osState.mousePos);
			Render(m->load, osState.mousePos);

			// Draw separators
			glColor3f(1, 1, 1);
			glLineWidth(UIBorderThickness);
			glBegin(GL_LINES);
			glVertex2f(GetTopRight(m->save.rectangle).x, m->save.rectangle.bottom + m->save.rectangle.height / 4);
			glVertex2f(GetTopRight(m->save.rectangle).x, m->save.rectangle.bottom + m->save.rectangle.height * 3 / 4);
			glColor3f(0, 0, 0);
			glVertex2f(m->background.left, m->background.bottom);
			glVertex2f(GetTopRight(m->background).width, m->background.bottom);
			glEnd();
			AssertOpenGL();
		}

		// @SECTION - Render the colour panel
		if(GetColourPanel()->display == true) {
			auto* panel = GetColourPanel();

			SetGUIProjectionMatrix();
			DrawRectangleFull(UnpackRectangle(panel->frame), 255, 255, 255, 1); // Draw the frame
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
				DrawCircleBorder(UnpackVector(panel->currentColour.wheelSelection), 10, UIBorderThickness, 0, 0, 0);

				AssertOpenGL();
			}

			// Get the current wheel colour selection
			colour currentColour = ConvertColourPanelColourToRGB(panel->currentColour);
			colour favouriteColours[GetArrayLength(panel->favourites)];
			ForAll(GetArrayLength(panel->favourites)) {
				if(ColourPanelColourIsInited(panel->favourites[it]) == true)
					favouriteColours[it] = ConvertColourPanelColourToRGB(panel->favourites[it]);
				else {
					favouriteColours[it].red = 1.0f;
					favouriteColours[it].green = 1.0f;
					favouriteColours[it].blue = 1.0f;
				}
			}
			
			// Draw colour slider
			{
				auto rec = GetColourPanelSliderRectangle();
				glBegin(GL_QUADS);
				glColor3f(0, 0, 0);
				glVertex2f(rec.left, rec.bottom);
				glVertex2f(rec.left + rec.width, rec.bottom);
				glColor3f(UnpackColourF32(currentColour));
				glVertex2f(rec.left + rec.width, rec.bottom + rec.height);
				glVertex2f(rec.left, rec.bottom + rec.height);
				glEnd();
				glLineWidth(1);
				DrawRectangleBorder(rec.left, rec.bottom, rec.width, rec.height, UIBorderThickness, 0, 0, 0);

				// Draw selection
				DrawRectangleBorder(rec.left - 3, panel->currentColour.sliderCenterY - 5, rec.width + 6, 10, UIBorderThickness, 0, 0, 0);
			}

			// RGB panels
			{
				// Red
				DrawRectangleBorder(UnpackRectangle(panel->red.container), UIBorderThickness, 0, 0, 0);
				Render(panel->red);
				RenderText("Red", Null, GetTopRight(panel->red.container).x + ColourPanelEdgeOffset, GetTextRectangle(panel->red).bottom, panel->red.textHeight, false);

				// Green
				DrawRectangleBorder(UnpackRectangle(panel->green.container), UIBorderThickness, 0, 0, 0);
				Render(panel->green);
				RenderText("Green", Null, GetTopRight(panel->green.container).x + ColourPanelEdgeOffset, GetTextRectangle(panel->green).bottom, panel->green.textHeight, false);

				// Blue
				DrawRectangleBorder(UnpackRectangle(panel->blue.container), UIBorderThickness, 0, 0, 0);
				Render(panel->blue);
				RenderText("Blue", Null, GetTopRight(panel->blue.container).x + ColourPanelEdgeOffset, GetTextRectangle(panel->blue).bottom, panel->blue.textHeight, false);
			}

			// Display RGB as a single number in hexadecimal
			if(panel->updatingCurrentColour == true || panel->updatingSlider == true)
				UpdateColourPanelRGBHexText(panel->currentColour);
			DrawRectangleBorder(UnpackRectangle(panel->hex.container), UIBorderThickness, 0, 0, 0);
			Render(panel->hex);

			// Display final colour
			f32 finalColourRight = Null;
			{
				auto wheel = GetColourPanelWheelRectangle();
				f32  radius = ColourPanelFavouritesLayerHeight / 2;
				f32  centerX = wheel.left + radius;
				f32  centerY = wheel.bottom - ColourPanelEdgeOffset - radius;
				
				DrawCircleFull(centerX, centerY, radius, UnpackColourUI8(currentColour), 1.0f);
				DrawCircleBorder(centerX, centerY, radius, UIBorderThickness, 0, 0, 0);

				finalColourRight = centerX + radius;
			}

			// Favourites
			{
				f32 start = panel->frame.left + ColourPanelEdgeOffset;
				f32 end = panel->save.rectangle.left - ColourPanelEdgeOffset;
				ui8 count = GetArrayLength(panel->favourites) + 1;
				auto* layouts = GetUIElementLayouts(start, end, count, ColourPanelFavouritesLayerHeight,
																						ColourPanelFavouritesLayerHeight / 2, ColourPanelFavouritesLayerHeight / 2, ColourPanelFavouritesLayerHeight / 2,
																						ColourPanelFavouritesLayerHeight / 2, ColourPanelFavouritesLayerHeight / 2, ColourPanelFavouritesLayerHeight / 2);
				f32 centerY = GetColourPanelWheelRectangle().bottom - ColourPanelEdgeOffset - ColourPanelFavouritesLayerHeight / 2;
				FromTo(1, count) {
					auto* l = layouts + it;
					DrawCircleFull(l->center, centerY, l->size / 2, UnpackColourUI8(favouriteColours[it - 1]), 1);
					DrawCircleBorder(l->center, centerY, l->size / 2, UIBorderThickness, 0, 0, 0);
				}
				
				if(panel->favouriteSelected != Null) {
					Assert(panel->favouriteSelected <= GetArrayLength(panel->favourites));
					auto* l = layouts + panel->favouriteSelected;
					DrawCircleBorder(l->center, centerY, l->size * 0.6f, UIBorderThickness, 0, 0, 0);
				}
				
				FreeUIElementLayouts(layouts);

				Render(panel->save, osState.mousePos);
				DrawRectangleBorder(UnpackRectangle(panel->save.rectangle), UIBorderThickness, 0, 0, 0);
			}

			// OK & cancel buttons
			DrawRectangleBorder(UnpackRectangle(panel->ok.rectangle), UIBorderThickness, 0, 0, 0);
			DrawRectangleBorder(UnpackRectangle(panel->cancel.rectangle), UIBorderThickness, 0, 0, 0);
			Render(panel->ok, osState.mousePos);
			Render(panel->cancel, osState.mousePos);
		}
		
		// Draw cursor if needed
		if(TextIsBeingUpdated() == true) {
			SetGUIProjectionMatrix();
			if(TitleIsBeingUpdated() == false)
				SetCanvasProjetionMatrix();

			f32 alpha = GetCursorAlphaValue();
			vector cursorPos = GetTextRectangle(*GetCurrentTextBody()).pos + GetCursorPos();
			
			glLineWidth(2);
			glColor4f(0, 0, 0, alpha);
			glBegin(GL_LINES);
			glVertex2f(cursorPos.x, cursorPos.y);
			glVertex2f(cursorPos.x, cursorPos.y + GetCurrentTextBody()->textHeight);
			glEnd();
			AssertOpenGL();
		}

		Win32EndGUIUpdateLoop(osState);
	}

	return 0;
}