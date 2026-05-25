#ifndef APAD_ERROR
#define APAD_ERROR

#include "apad_intrinsics.h"

// ******************** Generic ******************** //

dll_import void DisplayError(const char* string); // Prints the string without adding anything

dll_import void ExitProgram(bool error);
dll_import void RegisterExitFunction(void (*function)()); // Function to be called within ExitProgram()

/******************** Assertions ******************** 

#define APAD_DEBUGGER_ASSERTIONS for use in a debugger, do NOT otherwise as they will have no effect.
If the macro is not defined, assertions set the global error and record the failed condition in a global string.
Call SetExitIfAssertionHit(true) to have assertions stop program execution, otherwise will continue by default to allow client code to handle as seen fit.
Assertions will be printed in command line programs and displayed in a message box in GUI programs.

*****************************************************/

// User assertions
dll_import bool AssertionWasHit();
dll_import void ClearAssertionHit();
dll_import void SetExitIfAssertionHit(bool b); // True by default

// Set behaviour of API-only internal assertions
dll_import void SetDisplayAPIAssertions(bool b); // True by default
dll_import void SetCallExitInAPIAssertions(bool b);  // True by default

// Assert()
#include <intrin.h> // For __debugbreak()
#ifdef APAD_DEBUGGER_ASSERTIONS

// Will break into the debugger in debug mode and stop and exit program execution in release mode
#define Assert(_condition) { \
  if(!(_condition)) \
		__debugbreak(); \
}

#else

// If IsExitIfErrorSet() == true, will exit program execution. Otherwise, need to 
// manually check afterwards for errors and manually decide execution from there.
// IsExitIfErrorSet() == false by default.
#include <stdio.h> // For sprintf
#define Assert(_condition) { \
	ClearAssertionHit(); \
	if(!(_condition)) { \
	  dll_import bool AssertionHit; \
		AssertionHit = true; \
		\
	  char buffer[256] = {}; \
		program_external const char* GetFileNameAndExtension(const char*); \
	  sprintf(buffer, "Assertion failed \
	  								 \n  [Condition] %s \
	  								 \n  [File]      %s \
	  								 \n  [Line]      %lu", #_condition, GetFileNameAndExtension(__FILE__), __LINE__); \
	  DisplayError((const char*)buffer); \
		\
		dll_import program_external bool PrintAssertionsBacktrace; \
		if(PrintAssertionsBacktrace == true) { \
			dll_import program_external void Win32PrintStackBackTrace(); \
			Win32PrintStackBackTrace(); \
		} \
		\
		dll_import program_external bool CallExitInExternalAssertion; \
	  if(CallExitInExternalAssertion == true) \
	    ExitProgram(true); \
	} \
}

#endif

// AssertRet()
#define AssertRet(_condition) { \
	Assert(_condition); \
	if(AssertionWasHit() == true) \
	  return; \
}

// AssertRetType()
#define AssertRetType(_condition, _retValue) { \
	Assert(_condition); \
	if(AssertionWasHit() == true) \
	  return (_retValue); \
}

// InvalidCodePath
#define InvalidCodePath Assert(false)

#endif