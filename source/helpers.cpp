#include <windows.h>
#include <gl\gl.h>
#include "apad_error.h"
#include "apad_intrinsics.h"
#include "apad_maths.h"
#include "apad_string.h"
#include "helpers.h"

program_local void WriteTextLineHor(ui16 x, ui16 y, ui8 height) {
	glVertex2f(x, y);
	glVertex2f(x + height, y);
}

program_local void WriteTextLineVert(ui16 x, ui16 y, ui8 height) {
	glVertex2f(x, y);
	glVertex2f(x, y + height);
}

program_external point GetNoteTextStart(note* n) {
	Assert(n != Null);
	point p = {};
	p.x = n->background.left + NoteTextBorder;
	p.y = n->background.bottom + n->background.height - NoteTextBorder - NoteTextHeight;
	return p;
}

program_external note* GetNoteBeingWritten() {
	return state.notes.beingWritten;
}

program_external bool NoteHasText(note* n) {
	Assert(n != Null);
	Assert(IsValid(n->textMemory) == true);
	return n->textMemory.size > 0;
}

program_external char* GetNoteText(note* n) {
	Assert(n != Null);
	Assert(IsValid(n->textMemory) == true);
	return (char*)n->textMemory.memory;
}

program_external void BeginNoteWriting(note* n) {
	Assert(n != Null);
	Assert(IsValid(n->textMemory) == true);
	Assert(NoteIsBeingWritten() == false);
	state.notes.beingWritten = n;
}

program_external void AddNoteText(char c, note* n) {
	Assert(n != Null);
	Assert(IsValid(n->textMemory) == true);
	if(NoteHasText(n) == true)
		n->textMemory.size -= 1; // Remove \0
	char string[] = { c, '\0' };
	PushString(string, true, n->textMemory); // addEOS true since the '\0' in string[] won't be pushed
}

program_external void EndNoteWriting() {
	Assert(NoteIsBeingWritten() == true);
	state.notes.beingWritten = Null;
}

program_external bool NoteIsBeingWritten() {
	return state.notes.beingWritten != Null;
}

program_external f32 UI8ColourToF32(ui8 u) {
	return (f32)u / 255;
}

#include <windows.h>
#include <gl\gl.h>
program_external void DrawRectangle(ui16 left, ui16 bottom, ui16 width, ui16 height, ui8 r, ui8 g, ui8 b) {
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
}

program_external text_box WriteText(const char* string, ui16 x, ui16 y, ui8 height) {
	Assert(string != Null);
	
	text_box ret = {};
	ret.edges.left = x;
	ret.edges.bottom = y;
	
	auto length = GetStringLength(string);
	ui16 nextX = x;
	ui16 nextY = y;
	glColor3f(1, 0, 0);
	glLineWidth(3);
	glBegin(GL_LINES);
	ForAll(length) {
		char c = string[it];
		switch(c) {
			case(' '): break;
			
			case('\n'): {
				nextY -= height * 1.5f; 
				ret.edges.bottom = nextY;
				nextX = x; 
			} break;
			
			case('\b'): {
				WriteTextLineVert(nextX + height / 2, nextY + height / 4, height / 2);
				WriteTextLineHor(nextX + height / 4, nextY + height / 2, height / 2);
			} break;
			
			#if 0
			case('\t'): {
				if(it > 0 && string[it - 1] == '\b')
					string[it - 1] = ' ';
			} break;
			#endif
			
			case('a'):
			case('A'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineVert(nextX + height, nextY, height);
				WriteTextLineHor(nextX, nextY + height, height);
				WriteTextLineHor(nextX, nextY + height / 2, height);
			} break;
			
			case('b'):
			case('B'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineVert(nextX + height, nextY, height);
				
				WriteTextLineHor(nextX, nextY + height, height);
				WriteTextLineHor(nextX, nextY + height / 2, height);
				WriteTextLineHor(nextX, nextY, height);
			} break;
			
			case ('c'):
			case ('C'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineHor(nextX, nextY + height, height);
				WriteTextLineHor(nextX, nextY, height);
			} break;
			
			case ('d'):
			case ('D'): {
				WriteTextLineVert(nextX, nextY, height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height, nextY + height / 2);
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height, nextY + height / 2);
			} break;
			
			case ('e'):
			case ('E'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineHor(nextX, nextY + height, height);
				WriteTextLineHor(nextX, nextY + height / 2, height);
				WriteTextLineHor(nextX, nextY, height);
			} break;
			
			case ('f'):
			case ('F'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineHor(nextX, nextY + height, height);
				WriteTextLineHor(nextX, nextY + height / 2, height);
			} break;
			
			case ('g'):
			case ('G'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineHor(nextX, nextY + height, height);
				WriteTextLineHor(nextX, nextY, height);
				glVertex2f(nextX + height, nextY);
				glVertex2f(nextX + height, nextY + height / 2);
			} break;
			
			case ('h'):
			case ('H'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineVert(nextX + height, nextY, height);
				WriteTextLineHor(nextX, nextY + height / 2, height);
			} break;
			
			case ('i'):
			case ('I'): {
				WriteTextLineVert(nextX + height / 2, nextY, height);
			} break;
			
			case ('j'):
			case ('J'): {
				WriteTextLineVert(nextX + height, nextY, height);
				WriteTextLineHor(nextX, nextY, height);
				WriteTextLineHor(nextX, nextY + height, height);
			} break;
			
			case ('k'):
			case ('K'): {
				WriteTextLineVert(nextX, nextY, height);
				glVertex2f(nextX, nextY + height / 2);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX, nextY + height / 2);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('l'):
			case ('L'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineHor(nextX, nextY, height);
			} break;

			case ('m'):
			case ('M'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineVert(nextX + height, nextY, height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY + height);
			} break;
			
			case ('n'):
			case ('N'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineVert(nextX + height, nextY, height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height, nextY);
			} break;

			case ('o'):
			case ('O'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineVert(nextX + height, nextY, height);
				WriteTextLineHor(nextX, nextY + height, height);
				WriteTextLineHor(nextX, nextY, height);
			} break;
			
			case ('p'):
			case ('P'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineHor(nextX, nextY + height, height);
				WriteTextLineHor(nextX, nextY + height / 2, height);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX + height, nextY + height / 2);
			} break;
			
			case ('q'):
			case ('Q'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineVert(nextX + height / 2, nextY, height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height / 2, nextY + height);
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height / 2, nextY);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('r'):
			case ('R'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineHor(nextX, nextY + height, height);
				WriteTextLineHor(nextX, nextY + height / 2, height);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX + height, nextY + height / 2);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('s'):
			case ('S'): {
				WriteTextLineHor(nextX, nextY + height, height);
				WriteTextLineHor(nextX, nextY + height / 2, height);
				WriteTextLineHor(nextX, nextY, height);
				glVertex2f(nextX, nextY + height / 2);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height, nextY);
				glVertex2f(nextX + height, nextY + height / 2);
			} break;
			
			case ('t'):
			case ('T'): {
				WriteTextLineVert(nextX + height / 2, nextY, height);
				WriteTextLineHor(nextX, nextY + height, height);
			} break;
			
			case ('u'):
			case ('U'): {
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineVert(nextX + height, nextY, height);
				WriteTextLineHor(nextX, nextY, height);
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
				WriteTextLineVert(nextX, nextY, height);
				WriteTextLineVert(nextX + height, nextY, height);
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
				WriteTextLineHor(nextX, nextY + height, height);
				WriteTextLineHor(nextX, nextY, height);
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height, nextY + height);
			} break;
			
			default: break;
		}
		
		ret.edges.width = Max(ret.edges.width, nextX + height - ret.edges.left);
			
		if(c != '\n')
			nextX += height * 1.5f;
	}
	glEnd();
	
	ret.edges.height = y + height - ret.edges.bottom;
	ret.cursorLeft = nextX;
	Assert(ret.edges.width != 0);
	Assert(ret.edges.height != 0);
	
	return ret;
}