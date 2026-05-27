#include <windows.h>
#include <gl\gl.h>
#include "apad_error.h"
#include "apad_intrinsics.h"
#include "apad_string.h"

program_external void WriteText(const char* string, ui16 x, ui16 y, ui8 height) {
	Assert(string != Null);
	
	auto length = GetStringLength(string);
	ui16 nextX = x;
	ui16 nextY = y;
	glColor3f(1, 0, 0);
	glLineWidth(3);
	glBegin(GL_LINES);
	ForAll(length) {
		char c = string[it];
		if(c == 'a' || c == 'A') {
			glVertex2f(nextX, nextY);
			glVertex2f(nextX, nextY + height);
			
			glVertex2f(nextX, nextY + height);
			glVertex2f(nextX + height, nextY + height);
			
			glVertex2f(nextX + height, nextY + height);
			glVertex2f(nextX + height, nextY);
			
			glVertex2f(nextX, nextY + height / 2);
			glVertex2f(nextX + height, nextY + height / 2);
		}
		else if(c == 'b' || c == 'B') {
			glVertex2f(nextX, nextY);
			glVertex2f(nextX, nextY + height);
			
			glVertex2f(nextX, nextY + height);
			glVertex2f(nextX + height, nextY + height);
			
			glVertex2f(nextX + height, nextY + height);
			glVertex2f(nextX + height, nextY);
			
			glVertex2f(nextX, nextY + height / 2);
			glVertex2f(nextX + height, nextY + height / 2);
			
			glVertex2f(nextX, nextY);
			glVertex2f(nextX + height, nextY);
		}
		else if(c == 'o' || c == 'O') {
			glVertex2f(nextX, nextY);
			glVertex2f(nextX, nextY + height);
			
			glVertex2f(nextX, nextY + height);
			glVertex2f(nextX + height, nextY + height);
			
			glVertex2f(nextX + height, nextY + height);
			glVertex2f(nextX + height, nextY);
			
			glVertex2f(nextX, nextY);
			glVertex2f(nextX + height, nextY);
		}
		else if(c == 'l' || c == 'L') {
			glVertex2f(nextX, nextY);
			glVertex2f(nextX, nextY + height);
			
			glVertex2f(nextX, nextY);
			glVertex2f(nextX + height, nextY);
		}
		
		nextX += height * 1.5f;
	}
	glEnd();
}