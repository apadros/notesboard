#ifdef CopyMemory
#undef CopyMemory
#endif

#ifndef APAD_MEMORY_H
#define APAD_MEMORY_H

#include "apad_base_types.h"
#include "apad_intrinsics.h"

// ******************** Generic ******************** //

#define 							 KiB(value) ((value) * 1024)
#define 							 MiB(value) (KiB(value) * 1024)
#define 							 GiB(value) (MiB(value) * 1024)

#define 							 MovePtr(_ptr, _bytes) (_ptr) = (decltype(_ptr))((ui8*)(_ptr) + (_bytes))
#define 							 CastMemMovePtr(_mem, _dataType) ((_dataType*)(_mem)); MovePtr(_mem, sizeof(_dataType))
#define 							 ReadMemMovePtr(_mem, _dataType) *CastMemMovePtr(_mem, _dataType)

dll_import void ClearMemory(void* memory, ui32 size);
#define 							 ClearStruct(_s) ClearMemory(&(_s), sizeof(_s))
dll_import void CopyMemory(void* source, ui32 size, void* destination);

// ******************** Memory blocks ******************** //

struct memory_block {
  void* memory;
  ui32  size;
	ui32  capacity; // Stack functionality, will == 0 if not used this way
};
#define NullMemoryBlock memory_block()

typedef memory_block memory_stack;

dll_import memory_block AllocateMemory(ui32 size);
dll_import void*        GetMemory(memory_block block);
dll_import void         FreeMemory(memory_block& block); // Clears block afterwards
dll_import bool         IsValid(memory_block block);
dll_import void         SetInvalid(memory_block& block);

// ******************** Stack functionality ******************** //

dll_import memory_stack AllocateStack(ui32 capacity = Null);
dll_import void 				FreeStack(memory_stack& stack);

// All of these will allocate a new stack with a minimum of 2x capacity if not enough space is available for the push.
// As such it is strongly discouraged to store pointers into stack memory and to treat it as a single block.
dll_import void*  			Push(ui32 size, memory_stack& stack);
dll_import void*			  Push(void* memory, ui32 size, memory_stack& stack); 
#define                 PushInstance(_inst, _stack) \
												  Push(&(_inst), sizeof(_inst), (_stack))
#define 								PushStruct(_structType, _stack) \
													(_structType*)Push(sizeof(_structType), (_stack))

dll_import void 				ResetStack(memory_stack& stack);

#endif