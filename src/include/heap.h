/***************************************************************************
 heap.h Copyright (C) 1999 Magnus Reftel, Christoph Reichenbach


 This program may be modified and copied freely according to the terms of
 the GNU general public license (GPL), as long as the above copyright
 notice and the licensing information contained herein are preserved.

 Please refer to www.gnu.org for licensing details.

 This work is provided AS IS, without warranty of any kind, expressed or
 implied, including but not limited to the warranties of merchantibility,
 noninfringement, and fitness for a specific purpose. The author will not
 be held liable for any damage caused by this work or derivatives of it.

 By using this source code, you agree to the licensing terms as stated
 above.


 Please contact the maintainer for bug reports or inquiries.

 Current Maintainer:

    Christoph Reichenbach (CJR) [jameson@linuxgames.com]

***************************************************************************/

#ifndef _SCI_HEAP_H
#define _SCI_HEAP_H

#include <resource.h>

typedef guint16 heap_ptr;


typedef struct
{
	byte* start;
	byte* base;
	int first_free;
	int old_ff;
} heap_t;

heap_t*
heap_new();
/* Allocates a new heap.
** Parameters: (void)
** Returns   : (heap_t *) A new 0xffff-sized heap
*/

void
heap_del(heap_t* h);
/* Frees an allocated heap
** Parameters: (heap_t *) h: The heap to unallocate
** Returns   : (void)
*/

int
heap_largest(heap_t* h);
/* Returns the block size of the largest free block on the heap
** Parameters: (heap_t *) h: The heap to check
** Returns   : (int) The size of the largest free block
*/

heap_ptr
heap_allocate(heap_t* h, int size);
/* Allocates memory on a heap.
** Parameters: (heap_t *) h: The heap to work with
**             (int) size: The block size to allocate
** Returns   : (heap_ptr): The heap pointer to the new block, or 0 on failure
*/

void
heap_free(heap_t* h, int m);
/* Frees allocated heap memory.
** Parameters: (heap_t *) h: The heap to work with
**             (int) m: The handle at which memory is to be unallocated
** Returns   : (void)
** This function automatically prevents fragmentation from happening.
*/

void
save_ff(heap_t* h);
/* Stores the current first free position
** Parameters: (heap_t *) h: The heap which is to be manipulated
** Returns   : (void)
** This function can be used to save the heap state for later restoration (see
** the next function)
*/

void
restore_ff(heap_t* h);
/* Restores the first free heap state
** Parameters: (heap_t *) h: The heap to restore
** Returns   : (void)
** Restoring the first free state will reset the heap to the position stored
** when save_ff() was called, if and only if none of the blocks allocated before
** save_ff() was called was ever freed ("ever" includes "before save_ff() was
** called").
*/

void
heap_dump_free(heap_t *h);
/* Dumps debugging information about the stack
** Parameters: (heap_t *) h: The heap to check
** Returns   : (void)
*/

#endif /* !_SCI_HEAP_H */
