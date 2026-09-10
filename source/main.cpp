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
	
	// Win32 Font API stuff
	#if 0
	{
		// auto file = LoadFile("%windir%\\Fonts\\arial.ttf");
		int ret = AddFontResourceA("arial.ttf");
		Assert(ret == 1);
		// SendMessage(HWND_BROADCAST, WM_FONTCHANGE, NULL, NULL);
		
		HWND windowHandle = Win32GetGUIWindowHandle();
		HDC dc = GetDC(windowHandle);
		BOOL B = TextOutA(dc, 500, 500, "sample text", 11);
		
		// @TODO - Add Gdi32.lib to build.bat before testing any of this
		
		{
			GLYPHMETRICS gm = {};
			MAT2 mt;
			auto mem = AllocateMemory(KiB(1));
			DWORD ret = GetGlyphOutlineA(dc, 'a', GGO_BITMAP, &gm, mem.size, mem.memory, &mt);
			Assert(ret != GDI_ERROR);
			int a= 01;
			Free(mem);
		}
		#if 0
		// Remove manually added font
		{
			auto ret = RemoveFontResourceA("arial.ttf");
			if(ret != 0)
				SendMessage(HWND_BROADCAST, WM_FONTCHANGE, NULL, NULL);
		}
		#endif
	}
	#endif
	
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
	state.titleBar = AllocateTextBody(0, GetTopMenu()->background.bottom - TitleBarHeight, Win32GetProgramWindowClientSize().width, (TitleBarHeight - TitleBarTextHeight) / 2, TitleBarTextHeight, Null, TextBodyFlagLetters);
	Insert("Title", GetLength("Title"), state.titleBar, 0);

	// Init toolbar
	{
		auto* tb = &state.toolBar;
		tb->background.left = 0;
		tb->background.bottom = 0;
		tb->background.width = ToolbarWidth;
		tb->background.height = GetTopMenu()->background.bottom - GetTitleBar()->container.height;

		// Init buttons, starting at the top
		button* buttons[] = { &tb->newNote, &tb->bulletPoint, &tb->noteTitle, &tb->colourPanel };
		ForAll(GetArrayLength(buttons)) {
			char* texts[] = { "Note", "Bullet point", "Note title", "Colour Panel" } ;
			f32 width = ToobalIconWidth;
			f32 height = ToobalIconWidth;
			f32 textHeight = ToolbarTextHeight;
			*(buttons[it]) = AllocateButton(tb->background.left + tb->background.width / 2 - width / 2, 
																	 tb->background.height - (ToolVerticalSpaceBetweenIcons + height + textHeight * 2) * (it + 1), 
																	 width, height, 
																	 texts[it], textHeight,
																	 ColourPanelButtonsHighlightRGBA);
		}
		
	}
	
	state.canvas.colour = CreateColourUI8(200, 200, 200);
	
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
					panel->savedMainColour = panel->mainColour;
					panel->savedShade = panel->shade;
				}
				
				if(TextIsBeingUpdated() == true)
					EndTextUpdate();
				
				FreeText(panel->red);
				FreeText(panel->green);
				FreeText(panel->blue);
				FreeText(panel->hex);
				FreeButtonText(panel->ok);
				FreeButtonText(panel->cancel);
				FreeButtonText(panel->save);
				
				panel->display = false;
			}
			else if(ButtonClicked(panel->save, osState) == true) { // Save clicked
				if(panel->favouriteSelected != Null) { // Store in currently selected favourite
					panel->favourites[panel->favouriteSelected - 1].colour = GetCurrentColourPanelColour();
					panel->favourites[panel->favouriteSelected - 1].inited = true;
				}
				else { // Grab next one available
					ForAll(GetArrayLength(panel->favourites)) {
						auto* f = panel->favourites + it;
						if(f->inited == false) {
							f->colour = GetCurrentColourPanelColour();
							f->inited = true;
							break;
						}
					}
				}
			}
			else if( // Increase or decrease RGB fields by 1
							ButtonClicked(panel->redArrowUp, osState) == true || ButtonClicked(panel->redArrowDown, osState) == true ||
						  ButtonClicked(panel->greenArrowUp, osState) == true || ButtonClicked(panel->greenArrowDown, osState) == true ||
						  ButtonClicked(panel->blueArrowUp, osState) == true || ButtonClicked(panel->blueArrowDown, osState) == true) 
			{
				button* buttons[] = { &panel->redArrowUp, &panel->redArrowDown, &panel->greenArrowUp, &panel->greenArrowDown, &panel->blueArrowUp, &panel->blueArrowDown };
				ForAll(GetArrayLength(buttons)) {
					auto* b = buttons[it];
					if(ButtonClicked(*b, osState) == true) {
						// Get the right text field string
						char* text = Null;
						if(it <= 1)
							text = GetText(panel->red);
						else if(it <= 3)
							text = GetText(panel->green);
						else
							text = GetText(panel->blue);
						
						ui32 i = StringToInt(text, Null);
						if(it % 2 == 0 && i < 255)
							i += 1;
						else if(it % 2 == 1 && i > 0)
							i -= 1;
							
						// Pad the string with zeroes
						if(i <= 9)
							text = Concatenate(2, "00", ToString(i));
						else if(i <= 99)
							text = Concatenate(2, "0", ToString(i));
						else
							text = ToString(i);
						
						// Update text field
						text_body* body = Null;
						if(it <= 1)
							body = &panel->red;
						else if(it <= 3)
							body = &panel->green;
						else
							body = &panel->blue;
						ClearText(*body);
						Insert(text, 3, *body, 0);
						
						// Free memory
						Free(text);
				
						// Update rest of the colour panel
						SetColourPanelColour(StringToInt(GetText(panel->red), Null), StringToInt(GetText(panel->green), Null), StringToInt(GetText(panel->blue), Null));
						SetColourPanelHexText();
						
						break;
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
			else if( // Begin outer colour wheel update
							Win32MouseLeftDownThisFrame(osState) == true && 
							Overlap(UnpackVector(GetCenter(GetColourPanelWheelRectangle())), UnpackVector(osState.mousePos), GetColourPanelWheelRectangle().height / 2) == true && 
							Overlap(UnpackVector(GetCenter(GetColourPanelWheelRectangle())), UnpackVector(osState.mousePos), GetColourPanelWheelRectangle().height / 2 - ColourWheelThickness) == false) 
			{
				panel->updatingMainColour = true;
				if(TextIsBeingUpdated() == true && GetCurrentTextBody() == &panel->hex)
					EndTextUpdate();
			}
			else if(panel->updatingMainColour == true) { // Update current colour
				if(osState.mouseLeftDown == true) {
					// Angle of current selection
					f32 angle = 0; // About the horizontal axis
					vector vec = osState.mousePos - GetCenter(GetColourPanelWheelRectangle());
					if(vec.x == 0)
						angle = vec.y > 0 ? 90 : 270;
					else if(vec.y == 0)
						angle = vec.x > 0 ? 0 : 180;
					else {
						f32 a = Magnitude(vec.x);
						f32 o = Magnitude(vec.y);
						angle = ArcTan(o / a);
						if(vec.x < 0 && vec.y > 0)
							angle = 180 - angle;
						else if(vec.x < 0 && vec.y < 0)
							angle += 180;
						else if(vec.x > 0 && vec.y < 0)
							angle = 360 - angle;
					}
					
					// Adjust angle to start from the vertical axis (red)
					angle -= 90;
					if(angle < 0)
						angle += 360;
					
					panel->mainColour = angle;
					
					SetColourPanelRGBHexText();
				}
				else
					panel->updatingMainColour = false;
			}
			else if( // Begin inner colour rectangle update
							Win32MouseLeftDownThisFrame(osState) == true && 
							Overlap(UnpackVector(osState.mousePos), UnpackRectangle(GetColourPanelRectangleRectangle())) == true)
				panel->updatingShade = true;
			else if(panel->updatingShade == true) { // Update shade rectangle
				if(osState.mouseLeftDown == true) {
					auto rec = GetColourPanelRectangleRectangle();
					vector vec = osState.mousePos - rec.pos;
					Clamp(vec.x, 0, rec.width);
					Clamp(vec.y, 0, rec.height);
					vec.x /= rec.width;
					vec.y /= rec.height;
					panel->shade = vec;
					SetColourPanelRGBHexText();
				}
				else
					panel->updatingShade = false;
			}
			else if(Win32MouseLeftDownThisFrame(osState) == true) { // Check for selection of favourite colours
				ForAll(GetArrayLength(panel->favourites)) {
					auto* f = panel->favourites + it;
					if(Overlap(UnpackVector(osState.mousePos), UnpackVector(f->center), f->radius) == true) {
						panel->favouriteSelected = it + 1;
						if(f->inited == true) {
							SetColourPanelColour(UnpackColourUI8(f->colour));
							SetColourPanelRGBHexText();
						}
						
						break;
					}
				}
			}
			else if( // Update current colour based on updates to RGB & hex text bodies
							IsBeingUpdated(panel->red) == true || IsBeingUpdated(panel->green) == true || IsBeingUpdated(panel->blue) == true || IsBeingUpdated(panel->hex) == true || 
							textUpdatePipelineData.bodyBeingUpdatedThisFrame == &panel->red || textUpdatePipelineData.bodyBeingUpdatedThisFrame == &panel->green ||
							textUpdatePipelineData.bodyBeingUpdatedThisFrame == &panel->blue || textUpdatePipelineData.bodyBeingUpdatedThisFrame == &panel->hex) 
			{ 
				if( // EndTextUpdate() was called within RunTextUpdatePipeline(). If ESC was pressed, return to what was there before
						textUpdatePipelineData.bodyBeingUpdatedThisFrame != Null && osState.escapePressed == true) 
				{
					panel->mainColour = panel->savedMainColour;
					panel->shade = panel->savedShade;
					SetColourPanelRGBHexText();
				}
				else if(IsBeingUpdated(panel->hex) == true && (osState.keyPressed != Null || osState.enterPressed == true)) { // Special case for hex field
					if((osState.keyPressed >= 'a' && osState.keyPressed <= 'f' || osState.keyPressed <= 'A' && osState.keyPressed >= 'F' || osState.keyPressed >= '0' && osState.keyPressed <= '9') == false) // Chars must be hex
						RemoveChar(panel->hex, GetCursorCharOffset() - 1);
				
					char* text = GetText(panel->hex);
					if(GetLength(text) == 7 && text[0] == '#') { // Update only with full #xxxxxx format
						auto string = AllocateString(text + 1);
						ui32 b = ConvertHexToUI32(string + 4);
						string[4] = '\0';
						ui32 g = ConvertHexToUI32(string + 2);
						string[2] = '\0';
						ui32 r = ConvertHexToUI32(string);
						
						SetColourPanelColour(r, g, b);
						SetColourPanelRGBText(r, panel->red);
						SetColourPanelRGBText(g, panel->green);
						SetColourPanelRGBText(b, panel->blue);
					}
				}
				else if(osState.keyPressed != Null || osState.backspacePressed == true || osState.enterPressed == true) { // Change of value
					// Check for new value and clamp between 0 and 255
					if(IsBeingUpdated(panel->red) == true || IsBeingUpdated(panel->green) == true || IsBeingUpdated(panel->blue) == true) {
						text_body* b = Null;
						if(IsBeingUpdated(panel->red) == true)
							b = &panel->red;
						else if(IsBeingUpdated(panel->green) == true)
							b = &panel->green;
						else
							b = &panel->blue;
						
						char* text = GetText(*b);
						ui32  i = StringToInt(text, Null);
						if(i > 255) {
							ClearText(*b);
							Insert("255", 3, *b, 0);
						}
						
						SetColourPanelColour(StringToInt(GetText(panel->red), Null), StringToInt(GetText(panel->green), Null), StringToInt(GetText(panel->blue), Null));
						SetColourPanelHexText();
					}	
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
					{
						auto* panel = GetColourPanel();
						
						ForAll(GetArrayLength(panel->favourites)) {
							auto* f = panel->favourites + it;
							PushInstance(f->inited, memory);
							if(f->inited == true) {
								PushInstance(f->colour.red.i, memory);
								PushInstance(f->colour.green.i, memory);
								PushInstance(f->colour.blue.i, memory);
							}
						}
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
					// Store custom colours
					{
						auto* panel = GetColourPanel();
						
						ForAll(GetArrayLength(panel->favourites)) {
							auto* f = panel->favourites + it;
							f->inited = false;
							ClearInstance(f->colour);
							
							f->inited = ReadMemMovePtr(data, bool);
							if(f->inited == true) {
								ui8 red = ReadMemMovePtr(data, ui8);
								ui8 green = ReadMemMovePtr(data, ui8);
								ui8 blue = ReadMemMovePtr(data, ui8);
								f->colour = CreateColourUI8(red, green, blue);
							}
						}
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
		if(ButtonClicked(GetToolBar()->newNote, osState) == true) { // Create new note
			auto pos = ConvertToCanvasSpace(0, osState.mousePos.y - NoteMinHeight / 2);
			auto* n = CreateNote(pos, Null, Null);
			state.notes.selected = n;
			state.notes.justCreated = true;
			state.notes.moving = true;
			goto label_rendering;
		}
		else if(NoteTextIsBeingUpdated() == true && ButtonClicked(GetToolBar()->bulletPoint, osState) == true) // Add a bullet point
			InsertCharAtCursor(BulletPointChar); // Will check viability first
		else if(GetCurrentNote() != Null && ButtonClicked(GetToolBar()->noteTitle, osState) == true) { // Add a title to the currently selected note
			auto* n = GetCurrentNote();
			if(NoteHasTitle(n) == false) {
				auto textRec = GetTextRectangle(n->text);
				n->title = AllocateTextBody(textRec.left, GetTopRight(textRec).y, NoteMinWidth, NoteTextBorder, NoteTitleTextHeight, Null, TextBodyFlagLetters);
				Insert("Title", GetLength("Title"), n->title, 0);
				UpdateNoteContainers(n);
			}
		}
		else if(GetColourPanel()->display == false && ButtonClicked(GetToolBar()->colourPanel, osState) == true) // Open colour panel
			OpenColourPanel();

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
		DrawRectangleFull(0, 0, canvas.width, canvas.height, UnpackColourUI8(state.canvas.colour), 1);
		
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
			
			button* buttons[] = { &tb->newNote, &tb->bulletPoint, &tb->noteTitle, &tb->colourPanel };
			ForAll(GetArrayLength(buttons)) { // Buttons
				auto* b = buttons[it];
				Render(*b, UIBorderThickness, osState.mousePos, false);
				f32 bottom = b->rectangle.bottom - b->textHeight * 2;
				RenderText(b->text, GetLength(b->text), GetCenter(b->rectangle).x, bottom, b->textHeight, true);
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
			Render(m->save, Null, osState.mousePos, true);
			Render(m->load, Null, osState.mousePos, true);

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

		// @SECTION - Render colour panel
		if(GetColourPanel()->display == true) {
			auto* panel = GetColourPanel();

			SetGUIProjectionMatrix();
			DrawRectangleFull(UnpackRectangle(panel->frame), 255, 255, 255, 1); // Draw the frame
			DrawRectangleBorder(UnpackRectangle(panel->frame), UIBorderThickness, 0, 0, 0);

			// Draw outer colour wheel and current colour selection
			{
				rectangle wheel = GetColourPanelWheelRectangle();
				f32* outer = GenerateCircularCoords(ColourWheelVertices, UnpackVector(GetCenter(wheel)), wheel.height / 2);
				f32* inner = GenerateCircularCoords(ColourWheelVertices, UnpackVector(GetCenter(wheel)), wheel.height / 2 - ColourWheelThickness);
				
				glBegin(GL_TRIANGLE_STRIP);
				ForAll(ColourWheelVertices) {
					f32 angle = it * 360 / ColourWheelVertices;
					
					colour c = GetColourPanelWheelColour(angle);
					glColor3f(UnpackColourF32(c));
					
					glVertex2f(inner[it * 2], inner[it * 2 + 1]);
					glVertex2f(outer[it * 2], outer[it * 2 + 1]);					
				}
				glColor3f(1.0f, 0, 0);
				glVertex2f(inner[0], inner[1]);
				glVertex2f(outer[0], outer[1]);
				glEnd();
				
				Win32FreeMemory(outer);
				Win32FreeMemory(inner);

				// Draw edges
				DrawCircleBorder(UnpackVector(GetCenter(wheel)), wheel.width / 2, UIBorderThickness, 0, 0, 0);
				DrawCircleBorder(UnpackVector(GetCenter(wheel)), wheel.width / 2 - ColourWheelThickness, UIBorderThickness, 0, 0, 0);
				
				// Draw current colour selection
				{
					vector pos = GetCenter(wheel) + CreateVector(-Sine(panel->mainColour), Cos(panel->mainColour)) * (wheel.height / 2 - ColourWheelThickness / 2);
					f32 radius = ColourWheelThickness / 2;
					DrawCircleFull(UnpackVector(pos), radius, 255, 255, 255, 1.0f);
					DrawCircleBorder(UnpackVector(pos), radius, UIBorderThickness, 0, 0, 0);
				}

				AssertOpenGL();
			}
			
			// Determine outer wheel colour
			colour outerWheelColour = GetColourPanelWheelColour(panel->mainColour);

			// Draw inner colour rectangle and current colour selection
			{
				rectangle rec = GetColourPanelRectangleRectangle();
				
				glBegin(GL_QUADS);
				glColor3f(0, 0, 0);
				glVertex2f(rec.left, rec.bottom);
				glColor3f(1.0f, 1.0f, 1.0f);
				glVertex2f(rec.left + rec.width, rec.bottom);
				glColor3f(UnpackColourF32(outerWheelColour));
				glVertex2f(rec.left + rec.width, rec.bottom + rec.height);
				glColor3f(UnpackColourF32(outerWheelColour));
				glVertex2f(rec.left, rec.bottom + rec.height);
				glEnd();
				
				DrawRectangleBorder(UnpackRectangle(rec), UIBorderThickness, 0, 0, 0);
				
				// Draw current colour selection
				{
					vector pos = CreateVector(rec.left + panel->shade.x * rec.width, rec.bottom + panel->shade.y * rec.height);
					f32 radius = ColourWheelThickness / 2;
					DrawCircleFull(UnpackVector(pos), radius, 255, 255, 255, 1.0f);
					DrawCircleBorder(UnpackVector(pos), radius, UIBorderThickness, 0, 0, 0);
				}

				AssertOpenGL();
			}
			
			// Draw final colour
			{
				// Final colour taking the shade into consideration
				auto colour = GetCurrentColourPanelColour();
				
				// Render
				auto wheel = GetColourPanelWheelRectangle();
				f32  radius = ColourPanelFavouritesLayerHeight / 4;
				f32* vertices = GenerateCircularCoords(ColourWheelVertices, UnpackVector(GetCenter(wheel)), radius);
				DrawCircleBorder(wheel.left + radius, wheel.bottom - radius, radius, UIBorderThickness, 0, 0, 0);
				DrawCircleFull(wheel.left + radius, wheel.bottom - radius, radius, UnpackColourUI8(colour), 1.0f);
				Win32FreeMemory(vertices);
			}
			
			// RGB & hex panels & buttons
			{
				// Red
				DrawRectangleBorder(UnpackRectangle(panel->red.container), UIBorderThickness, 0, 0, 0);
				Render(panel->red);
				RenderText("Red", Null, GetTopRight(panel->redArrowUp.rectangle).x + ColourPanelEdgeOffset, GetTextRectangle(panel->red).bottom, panel->red.textHeight, false);
				Render(panel->redArrowUp, UIBorderThickness, osState.mousePos, false);
				Render(panel->redArrowDown, UIBorderThickness, osState.mousePos, false);

				// Green
				DrawRectangleBorder(UnpackRectangle(panel->green.container), UIBorderThickness, 0, 0, 0);
				Render(panel->green);
				RenderText("Green", Null, GetTopRight(panel->greenArrowUp.rectangle).x + ColourPanelEdgeOffset, GetTextRectangle(panel->green).bottom, panel->green.textHeight, false);
				Render(panel->greenArrowUp, UIBorderThickness, osState.mousePos, false);
				Render(panel->greenArrowDown, UIBorderThickness, osState.mousePos, false);

				// Blue
				DrawRectangleBorder(UnpackRectangle(panel->blue.container), UIBorderThickness, 0, 0, 0);
				Render(panel->blue);
				RenderText("Blue", Null, GetTopRight(panel->blueArrowUp.rectangle).x + ColourPanelEdgeOffset, GetTextRectangle(panel->blue).bottom, panel->blue.textHeight, false);
				Render(panel->blueArrowUp, UIBorderThickness, osState.mousePos, false);
				Render(panel->blueArrowDown, UIBorderThickness, osState.mousePos, false);
				
				// if(panel->updatingCurrentColour == true || panel->updatingSlider == true)
				// 	SetColourPanelRGBHexText(panel->currentColour);
				DrawRectangleBorder(UnpackRectangle(panel->hex.container), UIBorderThickness, 0, 0, 0);
				Render(panel->hex);
				
				// Render arrows
				{
					button* arrows[] = { &panel->redArrowUp, &panel->redArrowDown, &panel->greenArrowUp, &panel->greenArrowDown, &panel->blueArrowUp, &panel->blueArrowDown };
					ForAll(GetArrayLength(arrows)) {
						button* a = arrows[it];
						f32 left = a->rectangle.left + a->rectangle.width / 3;
						f32 right = a->rectangle.left + a->rectangle.width * 2 / 3;
						f32 bottom = a->rectangle.bottom + a->rectangle.height / 3;
						f32 top = a->rectangle.bottom + a->rectangle.height * 2 / 3;
						glBegin(GL_TRIANGLE_FAN);
						glColor3f(1.0f, 0, 0);
						if(it % 2 == 0) {
							glVertex2f(left, bottom);
							glVertex2f(right, bottom);
							glVertex2f(left + (right - left) / 2, top);
						}
						else {
							glVertex2f(left, top);
							glVertex2f(right, top);
							glVertex2f(left + (right - left) / 2, bottom);
						}
						glEnd();
					}
				}
			}

			// Favourites
			{
				ForAll(GetArrayLength(panel->favourites)) {
					auto* f = panel->favourites + it;
					if(f->inited == true)
						DrawCircleFull(UnpackVector(f->center), f->radius, UnpackColourUI8(f->colour), 1);
					else
						DrawCircleFull(UnpackVector(f->center), f->radius, 255, 255, 255, 1);
					DrawCircleBorder(UnpackVector(f->center), f->radius, UIBorderThickness, 0, 0, 0);
				}
				
				if(panel->favouriteSelected != Null) {
					Assert(panel->favouriteSelected - 1 < GetArrayLength(panel->favourites));
					auto* f = panel->favourites + panel->favouriteSelected - 1;
					DrawCircleBorder(UnpackVector(f->center), f->radius * 1.25f, UIBorderThickness , 0, 0, 0);
				}
				
				Render(panel->save, UIBorderThickness, osState.mousePos, true);
				DrawRectangleBorder(UnpackRectangle(panel->save.rectangle), UIBorderThickness, 0, 0, 0);
			}

			// OK & cancel buttons
			Render(panel->ok, UIBorderThickness, osState.mousePos, true);
			Render(panel->cancel, UIBorderThickness, osState.mousePos, true);
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