#ifndef APAD_TIME_H
#define APAD_TIME_H

#include "apad_base_types.h"
#include "apad_intrinsics.h"
#include "apad_string_args.h"

struct date {
  ui8  dayOfTheWeek; // 1 -> 7
  ui8  day; 				 // 1 -> 31
  ui8  month; 			 // 1 -> 12
  ui16 year;
};

/* Supported date argument formats are:
 * - today
 * - dd/mm, dd/mm/yy and dd/mm/yyyy
 * - Day of the week: mon, tue, wed, thu, fri, sat & sun
 * - date[[+ | -]days] e.g. today+5
 */
												// Return string will be allocated on global API memory.
dll_import char* DateToString(date d); // Returned string format is dd/mm/yyyy, needs to be freed
dll_import date  GetDate(si32 offsetDays);
dll_import bool  IsDateAndValid(string_args(string)); 
dll_import date  StringToDate(string_args(string)); // IsDateAndValid() should always be called before this

typedef ui64 time_marker;
dll_export char* 			 GetTimeNow(); // Returns string with format hh:mm:ss, needs to be freed
dll_import time_marker GetTimeMarker();
dll_import f32 			   GetTimeElapsedMilli(time_marker markerStart, time_marker markerEnd);

#endif