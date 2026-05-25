#ifndef APAD_BASE_TYPES_H
#define APAD_BASE_TYPES_H

// Types 
// VC19 x64 compiler uses LLP64 data model
typedef bool			   		   b8;

typedef signed char		   	 si8;
typedef unsigned char	   	 ui8;
typedef signed short	   	 si16;
typedef unsigned short	 	 ui16;
typedef signed int		   	 si32;
typedef unsigned int	   	 ui32;
typedef signed long long   si64;
typedef unsigned long long ui64;

typedef float			     		 f32;
typedef double			   		 f64;

// Constants
#define Null  0
#define Null2 Null, Null
#define Null3 Null2, Null
#define Null4 Null3, Null
#define Null5 Null4, Null

#define UI8Max 0xFF
#define UI16Max 0xFFFF
#define UI32Max 0xFFFF'FFFF
#define UI64Max 0xFFFF'FFFF'FFFF'FFFF

#endif