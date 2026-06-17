#include <windows.h>
#include <gl\gl.h>
#include "apad_error.h"
#include "apad_intrinsics.h"
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_opengl.h"
#include "apad_string.h"
#include "apad_win32_gui.h"
#include "helpers.h"

program_local void RenderTextLineHor(ui16 x, ui16 y, ui8 height) {
	glVertex2f(x, y);
	glVertex2f(x + height, y);
}

program_local void RenderTextLineVert(ui16 x, ui16 y, ui8 height) {
	glVertex2f(x, y);
	glVertex2f(x, y + height);
}

program_external void SetCursorPos(f32 x, f32 y) {
	state.textUpdate.cursorPos.x = x;
	state.textUpdate.cursorPos.y = y;
}

program_external void BeginWriting(memory_stack* textMemory, rectangle* containerBackground, ui16 cursorHeight) {
	Assert(textMemory != Null);
	Assert(containerBackground != Null);
	Assert(cursorHeight != Null);
	state.textUpdate.textMemory = textMemory;
	state.textUpdate.containerBackground = containerBackground;
	state.textUpdate.cursorHeight = cursorHeight;
	state.textUpdate.cursorIndex = textMemory->size - 1;
}

program_external bool NoteMemoryIsInUse(note* n) {
	Assert(n != Null);
	return n->background.width != 0 && n->background.height != 0;
}

program_external void DrawBorder(rectangle& r) {
	glLineWidth(3);
	glBegin(GL_LINES);
	glColor3f(0, 0, 0);
	
	glVertex2f(r.left, r.bottom);
	glVertex2f(r.left, r.bottom + r.height);
	
	glVertex2f(r.left, r.bottom + r.height);
	glVertex2f(r.left + r.width, r.bottom + r.height);
	
	glVertex2f(r.left + r.width, r.bottom + r.height);
	glVertex2f(r.left + r.width, r.bottom);
	
	glVertex2f(r.left + r.width, r.bottom);
	glVertex2f(r.left, r.bottom);
	glEnd();
	AssertOpenGL();	
}

program_external bool MouseLeftClickThisFrame() {
	return state.mouse.lastLeftDown == false && state.mouse.leftDown == true;
}

program_external bool MouseIsWithinToolbar() {
	return Overlap(state.mouse.pos.x, state.mouse.pos.y, UnpackDimensions(state.toolBar.background));
}

program_external bool TitleIsBeingUpdated() {
	return TextIsBeingWritten() == true && state.textUpdate.containerBackground == &state.titleBar.background;
}

program_external void MoveCursor(si8 offset) {
	Assert(TextIsBeingWritten() == true);
	
	auto* tu = &state.textUpdate;
	Assert(tu->cursorIndex <= tu->textMemory->size - 1);
	
	if(offset < 0) {
		if(-offset >= tu->cursorIndex)
			tu->cursorIndex = 0;
		else
			tu->cursorIndex += offset;
	}
	else if(offset > 0) {
		if(tu->cursorIndex + offset >= tu->textMemory->size - 1)
			tu->cursorIndex = tu->textMemory->size - 1;
		else
			tu->cursorIndex += offset;
	}
}

program_external ui16 GetCharOffset(char* c) {
	Assert(TextIsBeingWritten() == true);
	return (ui16)((ui8*)c - (ui8*)state.textUpdate.textMemory->memory);
}

program_external char* FindChar(char c, ui16 pos, bool scanForward) {
	Assert(TextIsBeingWritten() == true);
	
	auto* tu = &state.textUpdate;
	if(scanForward == false && pos == 0)
		return Null;
	
	char* text = (char*)tu->textMemory->memory;
	ui32  start = scanForward == true ? pos : pos - 1;
	ui32  end = scanForward == true ? tu->textMemory->size - 1 : 0;
	FromTo(start, end) {
		if(text[it] == c)
			return text + it;
	}
	
	return Null;
}

program_external bool NoteIsBeingUpdated() {
	return TextIsBeingWritten() == true && state.notes.selected != Null && state.textUpdate.containerBackground == &state.notes.selected->background;
}

program_external void EndWriting() {
	ClearStruct(state.textUpdate);
}

program_external void AddText(char c) {
	auto* tu = &state.textUpdate;
	Assert(tu->textMemory != Null);
	Assert(IsValid(*(tu->textMemory)) == true);
	void* mem = Insert(sizeof(c), tu->cursorIndex, *tu->textMemory);
	*((char*)mem) = c;
	tu->cursorIndex += 1;
}

program_external bool TextIsBeingWritten() {
	return state.textUpdate.textMemory != Null;
}

program_external f32 UI8ColourToF32(ui8 u) {
	return (f32)u / 255;
}

program_external vector ConvertToCanvasSpace(f32 x, f32 y) {
	vector p = {};
	p.x = (x - state.canvas.translation.x) / state.canvas.scale;
	p.y = (y - state.canvas.translation.y) / state.canvas.scale;
	return p;
}

program_external vector ConvertToCanvasSpace(vector pos) {
	return ConvertToCanvasSpace(pos.x, pos.y);
}

program_external vector ConvertToViewportSpace(vector pos) {
	vector p = {};
	p.x = pos.x * state.canvas.scale + state.canvas.translation.x;
	p.y = pos.y * state.canvas.scale + state.canvas.translation.y;
	return p;
}

// @TODO - Export to API?
program_external void ResetProjectionMatrix() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	AssertOpenGL();
		
	auto size = Win32GetProgramWindowClientSize();
	Assert(size.width > 0 && size.height > 0);
	glOrtho(0, size.width, 0, size.height, -1, 1);
	AssertOpenGL();		
}

#include <windows.h>
#include <gl\gl.h>
program_external void DrawRectangle(f32 left, f32 bottom, f32 width, f32 height, ui8 r, ui8 g, ui8 b) {
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
	AssertOpenGL();
}

program_external void SetCanvasProjetionMatrix() {
	ResetProjectionMatrix();
	if(state.canvas.scale != 1.0f)
		glScalef(state.canvas.scale, state.canvas.scale, 1.0f);
	if(state.canvas.translation.x != 0 || state.canvas.translation.y != 0)
		glTranslatef(state.canvas.translation.x / state.canvas.scale, state.canvas.translation.y / state.canvas.scale, Null);
	AssertOpenGL();		
}

program_external rectangle RenderText(const char* string, f32 x, f32 y, f32 height, bool center) {
	Assert(string != Null);
	
	rectangle ret = {};
	ret.left = x;
	ret.bottom = y;
	
	auto length = GetStringLength(string);
	f32 xOffset = 0;
	if(center == true) {
		ui16 fullWidth = 0;
		ui16 nextX = 0;
		ForAll(length) {
			char c = string[it];
			if(c == '\n')
				nextX = 0;
			else {
				nextX += height * 1.5f;
				fullWidth = GetMax(fullWidth, nextX);
			}
		}
		xOffset = fullWidth / 2;
	}
	
	f32 nextX = x - xOffset;
	f32 nextY = y;
	glColor3f(1, 0, 0);
	glLineWidth(3);
	glBegin(GL_LINES);
	ForAll(length) {
		char c = string[it];
		switch(c) {
			case(' '): break;
			
			case('\n'): {
				nextY -= height * 1.5f; 
				ret.bottom = nextY;
				nextX = x; 
			} break;
			
			case('\b'): {
				RenderTextLineVert(nextX + height / 2, nextY + height / 4, height / 2);
				RenderTextLineHor(nextX + height / 4, nextY + height / 2, height / 2);
			} break;
			
			case('a'):
			case('A'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
			} break;
			
			case('b'):
			case('B'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
				RenderTextLineHor(nextX, nextY, height);
			} break;
			
			case ('c'):
			case ('C'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY, height);
			} break;
			
			case ('d'):
			case ('D'): {
				RenderTextLineVert(nextX, nextY, height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height, nextY + height / 2);
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height, nextY + height / 2);
			} break;
			
			case ('e'):
			case ('E'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
				RenderTextLineHor(nextX, nextY, height);
			} break;
			
			case ('f'):
			case ('F'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
			} break;
			
			case ('g'):
			case ('G'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY, height);
				glVertex2f(nextX + height, nextY);
				glVertex2f(nextX + height, nextY + height / 2);
			} break;
			
			case ('h'):
			case ('H'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
			} break;
			
			case ('i'):
			case ('I'): {
				RenderTextLineVert(nextX + height / 2, nextY, height);
			} break;
			
			case ('j'):
			case ('J'): {
				RenderTextLineVert(nextX + height, nextY, height);
				RenderTextLineHor(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
			} break;
			
			case ('k'):
			case ('K'): {
				RenderTextLineVert(nextX, nextY, height);
				glVertex2f(nextX, nextY + height / 2);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX, nextY + height / 2);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('l'):
			case ('L'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY, height);
			} break;

			case ('m'):
			case ('M'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY + height);
			} break;
			
			case ('n'):
			case ('N'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height, nextY);
			} break;

			case ('o'):
			case ('O'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY, height);
			} break;
			
			case ('p'):
			case ('P'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX + height, nextY + height / 2);
			} break;
			
			case ('q'):
			case ('Q'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height / 2, nextY, height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height / 2, nextY + height);
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height / 2, nextY);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('r'):
			case ('R'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX + height, nextY + height / 2);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('s'):
			case ('S'): {
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
				RenderTextLineHor(nextX, nextY, height);
				glVertex2f(nextX, nextY + height / 2);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height, nextY);
				glVertex2f(nextX + height, nextY + height / 2);
			} break;
			
			case ('t'):
			case ('T'): {
				RenderTextLineVert(nextX + height / 2, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
			} break;
			
			case ('u'):
			case ('U'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				RenderTextLineHor(nextX, nextY, height);
			} break;
			
			case ('v'):
			case ('V'): {
				glVertex2f(nextX + height / 2, nextY);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height / 2, nextY);
				glVertex2f(nextX + height, nextY + height);
			} break;
			
			case ('w'):
			case ('W'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('x'):
			case ('X'): {
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('y'):
			case ('Y'): {
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height / 2, nextY);
				glVertex2f(nextX + height / 2, nextY + height / 2);
			} break;
			
			case ('z'):
			case ('Z'): {
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY, height);
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height, nextY + height);
			} break;
			
			default: break;
		}
		
		ret.width = GetMax(ret.width, nextX + height - ret.left);
			
		if(c != '\n')
			nextX += height * 1.5f;
	}
	glEnd();
	AssertOpenGL();
	
	ret.height = y + height - ret.bottom;
	Assert(ret.width != 0);
	Assert(ret.height != 0);
	
	return ret;
}