#ifndef APAD_MEMORY_H
#define APAD_MEMORY_H

#include "apad_base_types.h"
#include "apad_intrinsics.h"

// ******************** Generic ******************** //

#define 				KiB(value) ((value) * 1024)
#define 				MiB(value) (KiB(value) * 1024)
#define 				GiB(value) (MiB(value) * 1024)

#define 				MovePtr(_ptr, _bytes) 					(_ptr) = (decltype(_ptr))((ui8*)(_ptr) + (_bytes))
#define 				CastMemMovePtr(_mem, _dataType) ((_dataType*)(_mem)); MovePtr(_mem, sizeof(_dataType))
#define 				ReadMemMovePtr(_mem, _dataType) *CastMemMovePtr(_mem, _dataType)

dll_import void Clear(void* memory, ui32 size);
#define 				ClearInstance(_s) Clear(&(_s), sizeof(_s))
dll_import void Copy(void* source, ui32 size, void* destination);
#define         CopyInstance(_s, _destination) Copy(&(_s), sizeof(_s), _destination)

// ******************** Memory block and offset ******************** //

struct memory_block {
  void* memory; // Never store this! Store the whole memory_block
  ui32  size;
};
#define NullMemoryBlock memory_block()

dll_import memory_block AllocateMemory(ui32 size); // No need to call as Push() will call it on first use unless strict memory capacity is required
dll_import void 			  Expand(memory_block& b); // Works for memory_stacks and memory_pools too. Will allocate new block with size or capacity * 2
dll_import void         Free(memory_block& block); // Clears block afterwards
dll_import void         Free(void* memory); // Only for memory allocated through this API or at the OS level
dll_import bool         IsValid(memory_block block);
dll_import void         SetInvalid(memory_block& block); // Will not free memory

// Use this to store pointers into memory_blocks since the latter's memory may be reallocated through its lifecycle
struct memory_offset {
	memory_block* block;
	ui32          offset;
};

dll_import memory_offset GetOffset(void* memory, memory_block& block);
dll_import void* 	 			 GetMemory(memory_offset offset); // Will return Null if offset is invalid
dll_import bool 				 IsValid(memory_offset offset);
dll_import void 				 SetInvalid(memory_offset& offset);

// ******************** Stack allocator ******************** //

struct memory_stack {
	memory_block memory; // memory.size treated as stack capacity.
	ui32         size;
};

dll_import memory_stack AllocateStack(ui32 capacity = Null);
dll_import void 				Free(memory_stack&);
dll_import ui32 				GetCapacity(memory_stack stack);
dll_import void* 				GetMemory(memory_stack stack);
dll_import void* 				Insert(ui32 size, ui32 offset, memory_stack&);
dll_import bool 				IsValid(memory_stack&);
dll_import void 				Pop(ui32 size, memory_stack&); // If size >= stack.size, stack.size will be set to 0

												// All of these will allocate a new stack with a minimum of 2x capacity if not enough space is available for the push.
												// As such it is strongly discouraged to store pointers into stack memory and to treat it as a single block.
dll_import void*  			Push(ui32 size, memory_stack& stack); // Will initialise stack on first use
dll_import void*			  Push(void* memory, ui32 size, memory_stack& stack); // Will initialise stack on first use
#define                 PushInstance(_inst, _stack) Push(&(_inst), sizeof(_inst), (_stack))
#define                 PushPointer(_ptr, _stack) 	Push((void*)(_ptr), sizeof(*(_ptr)), _stack);
#define 								PushType(_type, _stack) 		(_type*)Push(sizeof(_type), (_stack))

dll_import void  				Remove(ui32 size, ui32 offset, memory_stack& stack); // Will move contents beyond offset + size down to offset
dll_import void 				Reset(memory_stack& stack);

// ******************** Pool allocator ******************** //

// Do not store raw pointers into its memory, use GetOffset(void*, pool.memory) instead
struct memory_pool {
	memory_block memory;
	ui16         elementSize;
};

#define BeginMemoryPoolLoop(_pool, _allocatedBoolPtrID, _memPtrID) { \
					ForAll(_pool.memory.size / (sizeof(b8) + _pool.elementSize)) { \
					b8*   _allocatedBoolPtrID = (b8*)((ui8*)_pool.memory.memory + (sizeof(b8) + _pool.elementSize) * it); \
					void* _memPtrID = (ui8*)_allocatedBoolPtrID + 1;

#define EndMemoryPoolLoop() } }

dll_import memory_pool   AllocatePool(ui16 elementSize, ui16 count = Null); // Can leave count == Null to initialise
dll_import void* 				 // Do NOT store the returned raw pointer, call GetOffset(void*, pool.memory) instead.
												 Allocate( // AllocatePool() with valid elementSize must have been called beforehand.
																	 memory_pool& pool);
dll_import memory_offset Allocate(void* memory, ui16 size, memory_pool&); // AllocatePool() with valid elementSize must have been called beforehand
dll_import void 				 Deallocate(void* memory, memory_pool& pool); // Must be called before any other pool API call after respective Allocate() call to avoid internal memory reallocation
dll_import void 			 	 Deallocate(memory_offset offset, memory_pool& pool);
dll_import void 				 Free(memory_pool& pool);

#endif