/***************************************************************************
 resource.h Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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

    Christoph Reichenbach (CJR) [creichen@rbg.informatik.tu-darmstadt.de]

 History:

   990327 - created (CJR)

***************************************************************************/

#ifndef _SCI_RESOURCE_H_
#define _SCI_RESOURCE_H_

/*#define _SCI_RESOURCE_DEBUG */
/*#define _SCI_DECOMPRESS_DEBUG*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include <io.h>
#else /* !_WIN32 */
#define DLLEXTERN
#endif /* !_WIN32 */

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <assert.h>
#ifndef _DOS
#include <glib.h>
#else
#include <sci_dos.h>
#endif

#ifdef _MSC_VER
  #ifdef FREESCI_EXPORTS
  #define DLLEXTERN
  #else
  #define DLLEXTERN __declspec(dllimport)
  #endif
#else
#define DLLEXTERN
#endif

#define SCI_MAX_RESOURCE_SIZE 0x0400000
/* The maximum allowed size for a compressed or decompressed resource */


/*** RESOURCE STATUS TYPES ***/
#define SCI_STATUS_OK 0
#define SCI_STATUS_NOMALLOC 1


/*** INITIALIZATION RESULT TYPES ***/
#define SCI_ERROR_IO_ERROR 1
#define SCI_ERROR_EMPTY_OBJECT 2
#define SCI_ERROR_NO_RESOURCE_FILES_FOUND 3
/* neither resource.000 nor resource.001 were found */
#define SCI_ERROR_UNKNOWN_COMPRESSION 4
#define SCI_ERROR_DECOMPRESSION_OVERFLOW 5
/* decompression failed: Buffer overflow (wrong SCI version?)  */
#define SCI_ERROR_DECOMPRESSION_INSANE 6
/* sanity checks failed during decompression */
#define SCI_ERROR_RESOURCE_TOO_BIG 7
/* Resource size exceeds SCI_MAX_RESOURCE_SIZE */
#define SCI_ERROR_UNSUPPORTED_VERSION 8
#define SCI_ERROR_INVALID_SCRIPT_VERSION 9

#define SCI_ERROR_CRITICAL SCI_ERROR_NO_RESOURCE_FILES_FOUND
/* the first critical error number */

/*** SCI VERSION NUMBERS ***/
#define SCI_VERSION_AUTODETECT 0
#define SCI_VERSION_0 1
#define SCI_VERSION_01 2
#define SCI_VERSION_1 3
#define SCI_VERSION_32 4
#define SCI_VERSION_LAST SCI_VERSION_01
/* The last supported SCI version */

extern DLLEXTERN const char* SCI_Error_Types[];
extern DLLEXTERN const char* SCI_Version_Types[];
extern DLLEXTERN const char* Resource_Types[];
extern DLLEXTERN const char* resource_type_suffixes[]; /* Suffixes for SCI1 patch files */
extern DLLEXTERN const int sci_max_resource_nr[]; /* Highest possible resource numbers */


enum ResourceTypes {
  sci_view=0, sci_pic, sci_script, sci_text,
  sci_sound, sci_memory, sci_vocab, sci_font,
  sci_cursor, sci_patch, sci_bitmap, sci_palette,
  sci_cdaudio, sci_audio, sci_sync, sci_message,
  sci_map, sci_heap, sci_invalid_resource
};

#define sci0_last_resource sci_patch
/* Used for autodetection */


typedef guint8 byte;
typedef guint16 word;

struct resource_index_struct {
  unsigned short resource_id;
  unsigned int resource_location;
}; /* resource type as stored in the resource.map file */

typedef struct resource_index_struct resource_index_t;

struct resource_struct {
  unsigned short number;
  unsigned short type;
  guint16 id; /* contains number and type */
  guint16 length;
  unsigned char *data;
  unsigned char status;
}; /* for storing resources in memory */

typedef struct resource_struct resource_t;


extern DLLEXTERN int sci_version;
/* The version we are currently working with */

extern DLLEXTERN int max_resource;
extern DLLEXTERN resource_t *resource_map;


/**** FUNCTION DECLARATIONS ****/

#ifndef _GET_INT_16
#define _GET_INT_16

#ifdef WORDS_BIGENDIAN

gint16 getInt16(byte *d);

#else /* !WORDS_BIGENDIAN */

static inline gint16
getInt16(byte *d)
{
  return *d | (d[1] << 8);
}

#endif /* !WORDS_BIGENDIAN */
#define getUInt16(d) (guint16)(getInt16(d))
#endif
/* Turns a little endian 16 bit value into a machine-dependant 16 bit value
** Parameters: d: Pointer to the memory position from which to read
** Returns   : (gint16) The (possibly converted) 16 bit value
** getUInt16 returns the int unsigned.
*/


void *
_XALLOC(size_t size, char *file, int line, char *funct);
#ifdef __GNUC__
#define xalloc(size) \
        _XALLOC((size), __FILE__, __LINE__, __PRETTY_FUNCTION__)
#else /* !__GNUC__ */
#define xalloc(size) \
        _XALLOC((size), __FILE__, __LINE__, "")
#endif /* !__GNUC__ */
/* Tries to allocate memory, prints an error message if not successful.
** Parameters: size: Number of bytes to allocate
** Returns   : (void *) The address of the allocated memory block
*/


int loadResources(int version, int allow_patches);
/* Reads and parses all resource files in the current directory.
** Parameters: version: The SCI version to look for; use SCI_VERSION_AUTODETECT
**                      in the default case.
**             allow_patches: Set to 1 if external patches (those look like
**                            "view.101" or "script.093") should be applied
** Returns   : (int) Always 0.
** TODO: Add some error checking.
*/

void freeResources();
/* Frees all allocated resource memory.
** Parameters: none
** Returns   : (void)
*/

resource_t * findResource(unsigned short type,
                          unsigned short number);
/* Searches for a resource with the specified resource type and number.
** Parameters: type: The resource type (one of ResourceTypes)
**             number: The resource number (the "042" in "view.042")
** Returns   : (resource_t *) pointing to the resource, or NULL
**             if it could not be found.
*/

int decompress0(resource_t *result, int resh);
/* Decrypts resource data and stores the result for SCI0-style compression.
** Parameters : result: The resource_t the decompressed data is stored in.
**              resh  : File handle of the resource file
** Returns    : (int) 0 on success, one of SCI_ERROR_* if a problem was
**               encountered.
*/

int decompress01(resource_t *result, int resh);
/* Decrypts resource data and stores the result for SCI01-style compression.
** Parameters : result: The resource_t the decompressed data is stored in.
**              resh  : File handle of the resource file
** Returns    : (int) 0 on success, one of SCI_ERROR_* if a problem was
**               encountered.
*/

int decompress1(resource_t *result, int resh);
/* Decrypts resource data and stores the result for SCI1-style compression.
** Parameters : result: The resource_t the decompressed data is stored in.
**              resh  : File handle of the resource file
** Returns    : (int) 0 on success, one of SCI_ERROR_* if a problem was
**               encountered.
*/

int decrypt2(guint8* dest, guint8* src, int length, int complength);
/* Huffman token decryptor - defined in decompress0.c and used in decompress01.c
*/


#define SCI_MEMTEST memtest(__FILE__ ": line %d", __LINE__)

int
memtest(char *location, ...);
/* Allocates, manipulates, and frees some memory
** Parameters: (char *) location,... : The location to print
** Returns   : (int) 0
** This function calls malloc(), free(), and memfrob() or memset()
** to provocate segmentation faults caused by dynamic allocation bugs
** in previous parts of the code.
*/

void
sci_gettime(int *seconds, int *useconds);
/* Calculates the current time in seconds and microseconds
** Parameters: (int *) seconds: Pointer to the variable the seconds part of the
**                              current time will be stored in
**             (int *) useconds: Pointer to the variable the microseconds part
**                               of the current time will be stored in
** Returns   : (void)
** The resulting values must be relative to an arbitrary fixed point in time
** (typically 01/01/1970 on *NIX systems).
*/


/* The following was stolen and adapted from glib.h, and is partially
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 */
#if defined (__i386__) && defined (__GNUC__) && __GNUC__ >= 2
#define BREAKPOINT()          {__asm__ __volatile__ ("int $03"); }
#elif defined (__alpha__) && defined (__GNUC__) && __GNUC__ >= 2
#define BREAKPOINT()          {__asm__ __volatile__ ("bpt"); }
#else   /* !__i386__ && !__alpha__ */
#define BREAKPOINT() { fprintf(stderr, "Missed breakpoint in %s, line %d\n", __FILE__, __LINE__); exit(1); }
#endif  /* __i386__ */


#endif

