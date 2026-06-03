#include <windows.h>
#include <gl\gl.h>
#include "apad_error.h"
#include "apad_intrinsics.h"
#include "apad_string.h"

program_local void WriteTextLineHor(ui16 x, ui16 y, ui8 height) {
	glVertex2f(x, y);
	glVertex2f(x + height, y);
}

program_local void WriteTextLineVert(ui16 x, ui16 y, ui8 height) {
	glVertex2f(x, y);
	glVertex2f(x, y + height);
}

program_external ui16 WriteText(const char* string, ui16 x, ui16 y, ui8 height) {
	Assert(string != Null);
	
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
				nextX = x; 
			} break;
			
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
			
		if(c != '\n')
			nextX += height * 1.5f;
	}
	glEnd();
	
	return nextX - height * 0.5f;
}