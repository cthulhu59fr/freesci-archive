/***************************************************************************
 decompress1.c Copyright (C) 1999 The FreeSCI project


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

***************************************************************************/
/* Reads data from a resource file and stores the result in memory */


#include <engine.h>


void decryptinit3();
int decrypt3(guint8* dest, guint8* src, int length, int complength);


int decompress1(resource_t *result, int resh)
{
  guint16 compressedLength;
  guint16 compressionMethod;
  guint8 *buffer;
  guint8 tempid;

  if (read(resh, &tempid, 1) != 1)
    return SCI_ERROR_IO_ERROR;

  result->id = tempid;

  result->type = result->id &0x7f;
  if (read(resh, &(result->number), 2) != 2)
    return SCI_ERROR_IO_ERROR;

#ifdef WORDS_BIGENDIAN
  result->number = GUINT16_SWAP_LE_BE_CONSTANT(result->id);
#endif /* WORDS_BIGENDIAN */
  if ((result->number > 8191) || (result->type > sci_invalid_resource))
    return SCI_ERROR_DECOMPRESSION_INSANE;

  if ((read(resh, &compressedLength, 2) != 2) ||
      (read(resh, &(result->length), 2) != 2) ||
      (read(resh, &compressionMethod, 2) != 2))
    return SCI_ERROR_IO_ERROR;

#ifdef WORDS_BIGENDIAN
  compressedLength = GUINT16_SWAP_LE_BE_CONSTANT(compressedLength);
  result->length = GUINT16_SWAP_LE_BE_CONSTANT(result->length);
  compressionMethod = GUINT16_SWAP_LE_BE_CONSTANT(compressionMethod);
#endif

  /*  if ((result->length < 0) || (compressedLength < 0))
      return SCI_ERROR_DECOMPRESSION_INSANE; */
  /* This return will never happen in SCI0 or SCI1 (does it have any use?) */

  if ((result->length > SCI_MAX_RESOURCE_SIZE) ||
      (compressedLength > SCI_MAX_RESOURCE_SIZE))
    return SCI_ERROR_RESOURCE_TOO_BIG;

  if (compressedLength > 4)
    compressedLength -= 4;
  else { /* Object has size zero (e.g. view.000 in sq3) (does this really exist?) */
    result->data = 0;
    result->status = SCI_STATUS_NOMALLOC;
    return SCI_ERROR_EMPTY_OBJECT;
  }

  buffer = g_malloc(compressedLength);
  result->data = g_malloc(result->length);

  if (read(resh, buffer, compressedLength) != compressedLength) {
    g_free(result->data);
    g_free(buffer);
    return SCI_ERROR_IO_ERROR;
  };


#ifdef _SCI_DECOMPRESS_DEBUG
  fprintf(stderr, "Resource %i.%s encrypted with method SCI1/%hi at %.2f%%"
	  " ratio\n",
	  result->number, resource_type_suffixes[result->type], compressionMethod,
	  (result->length == 0)? -1.0 : 
	  (100.0 * compressedLength / result->length));
  fprintf(stderr, "  compressedLength = 0x%hx, actualLength=0x%hx\n",
	  compressedLength, result->length);
#endif

  switch(compressionMethod) {

  case 0: /* no compression */
    if (result->length != compressedLength) {
      g_free(result->data);
      result->data = NULL;
      result->status = SCI_STATUS_NOMALLOC;
      g_free(buffer);
      return SCI_ERROR_DECOMPRESSION_OVERFLOW;
    }
    memcpy(result->data, buffer, compressedLength);
    result->status = SCI_STATUS_OK;
    break;

  case 1: /* LZW */
    if (decrypt2(result->data, buffer, result->length, compressedLength)) {
      g_free(result->data);
      result->data = 0; /* So that we know that it didn't work */
      result->status = SCI_STATUS_NOMALLOC;
      g_free(buffer);
      return SCI_ERROR_DECOMPRESSION_OVERFLOW;
    }
    result->status = SCI_STATUS_OK;
    break;

  case 2: /* ??? */
    decryptinit3();
    if (decrypt3(result->data, buffer, result->length, compressedLength)) {
      g_free(result->data);
      result->data = 0; /* So that we know that it didn't work */
      result->status = SCI_STATUS_NOMALLOC;
      g_free(buffer);
      return SCI_ERROR_DECOMPRESSION_OVERFLOW;
    }
    result->status = SCI_STATUS_OK;
    break;

#if 0
  case 3: /* Some sort of Huffman encoding */
    if (decrypt2(result->data, buffer, result->length, compressedLength)) {
      g_free(result->data);
      result->data = 0; /* So that we know that it didn't work */
      result->status = SCI_STATUS_NOMALLOC;
      g_free(buffer);
      return SCI_ERROR_DECOMPRESSION_OVERFLOW;
    }
    result->status = SCI_STATUS_OK;
    break;
#endif

  case 3:
  case 4: /* NYI */
    fprintf(stderr,"Resource %d.%s: Warning: compression type #%d not yet implemented\n",
	    result->number, resource_type_suffixes[result->type], compressionMethod);
    g_free(result->data);
    result->data = NULL;
    result->status = SCI_STATUS_NOMALLOC;
    break;

  default:
    fprintf(stderr,"Resource %d.%s: Compression method SCI1/%hi not "
	    "supported!\n", result->number, resource_type_suffixes[result->type],
	    compressionMethod);
    g_free(result->data);
    result->data = 0; /* So that we know that it didn't work */
    result->status = SCI_STATUS_NOMALLOC;
    g_free(buffer);
    return SCI_ERROR_UNKNOWN_COMPRESSION;
  }

  g_free(buffer);
  return 0;
}

