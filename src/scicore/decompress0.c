/***************************************************************************
 decompress0.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Reads data from a resource file and stores the result in memory.
** This is for SCI version 0 style compression.
*/

#include <engine.h>

/* #define _SCI_DECOMPRESS_DEBUG */

/* 9-12 bit LZW encoding */
int decrypt1(guint8 *dest, guint8 *src, int length, int complength)
     /* Doesn't do length checking yet */
{
  /* Theory: Considering the input as a bit stream, we get a series of
  ** 9 bit elements in the beginning. Every one of them is a 'token'
  ** and either represents a literal (if < 0x100), or a link to a previous
  ** token (tokens start at 0x102, because 0x101 is the end-of-stream
  ** indicator and 0x100 is used to reset the bit stream decoder).
  ** If it's a link, the indicated token and the character following it are
  ** placed into the output stream. Note that the 'indicated token' may
  ** very well consist of a link-token-plus-literal construct again, so
  ** it's possible to represent strings longer than 2 recursively.
  ** If the maximum number of tokens has been reached, the bit length is
  ** increased by one, up to a maximum of 12 bits.
  ** This implementation remembers the position each token was print to in
  ** the output array, and the length of this token. This method should
  ** be faster than the recursive approach.
  */

  guint16 bitlen = 9; /* no. of bits to read (max. 12) */
  guint16 bitmask = 0x01ff;
  guint16 bitctr = 0; /* current bit position */
  guint16 bytectr = 0; /* current byte position */
  guint16 token; /* The last received value */
  guint16 maxtoken = 0x200; /* The biggest token */

  guint16 tokenlist[4096]; /* pointers to dest[] */
  guint16 tokenlengthlist[4096]; /* char length of each token */
  guint16 tokenctr = 0x102; /* no. of registered tokens (starts here)*/

  guint16 tokenlastlength = 0;

  guint16 destctr = 0;

  while (bytectr < complength) {

    guint32 tokenmaker = src[bytectr++] >> bitctr;
    if (bytectr < complength)
      tokenmaker |= (src[bytectr] << (8-bitctr));
    if (bytectr+1 < complength)
      tokenmaker |= (src[bytectr+1] << (16-bitctr));

    token = tokenmaker & bitmask;

    bitctr += bitlen - 8;

    while (bitctr >= 8) {
      bitctr -= 8;
      bytectr++;
    }

    if (token == 0x101) return 0; /* terminator */
    if (token == 0x100) { /* reset command */
      maxtoken = 0x200;
      bitlen = 9;
      bitmask = 0x01ff;
      tokenctr = 0x0102;
    } else {

      {
	int i;

	if (token > 0xff) {
	  tokenlastlength = tokenlengthlist[token]+1;
	  for (i=0; i< tokenlastlength; i++) {
	    dest[destctr++] = dest[tokenlist[token]+i];
	  }
	} else {
	  tokenlastlength = 1;
	  dest[destctr++] = token;
	}

      }

      if (tokenctr == maxtoken) {
	if (bitlen < 12) {
	  bitlen++;
	  bitmask <<= 1;
	  bitmask |= 1;
	  maxtoken <<= 1;
	} else continue; /* no further tokens allowed */
      }

      tokenlist[tokenctr] = destctr-tokenlastlength;
      tokenlengthlist[tokenctr++] = tokenlastlength;

    }
      
  }

  return 0;

}


/* Huffman-style token encoding */
/***************************************************************************/
/* This code was taken from Carl Muckenhoupt's sde.c, with some minor      */
/* modifications.                                                          */
/***************************************************************************/

/* decrypt2 helper function */
gint16 getc2(guint8 *node, guint8 *src,
	     guint16 *bytectr, guint16 *bitctr, int complength)
{
  guint16 next;

  while (node[1] != 0) {
    gint16 value = (src[*bytectr] << (*bitctr));
    (*bitctr)++;
    if (*bitctr == 8) {
      (*bitctr) = 0;
      (*bytectr)++;
    }

    if (value & 0x80) {
      next = node[1] & 0x0f; /* low 4 bits */
      if (next == 0) {
	guint16 result = (src[*bytectr] << (*bitctr));

	if (++(*bytectr) > complength)
	  return -1;
	else if (*bytectr < complength)
	  result |= src[*bytectr] >> (8-(*bitctr));

	result &= 0x0ff;
	return (result | 0x100);
      }
    }
    else { 
      next = node[1] >> 4;  /* high 4 bits */
    }
    node += next<<1;
  }
  return getInt16(node);
}

/* Huffman token decryptor */
int decrypt2(guint8* dest, guint8* src, int length, int complength)
     /* no complength checking atm */
{
  guint8 numnodes, terminator;
  guint8 *nodes;
  gint16 c;
  guint16 bitctr = 0, bytectr;

  numnodes = src[0];
  terminator = src[1];
  bytectr = 2+ (numnodes << 1);
  nodes = src+2;

  while (((c = getc2(nodes, src, &bytectr, &bitctr, complength))
	 != (0x0100 | terminator)) && (c >= 0)) {
    if (length-- == 0) return SCI_ERROR_DECOMPRESSION_OVERFLOW;

    *dest = (guint8)c;
    dest++;
  }

  return (c == -1) ? SCI_ERROR_DECOMPRESSION_OVERFLOW : 0;

}
/***************************************************************************/
/* Carl Muckenhoupt's decompression code ends here                         */
/***************************************************************************/

int decompress0(resource_t *result, int resh)
{
  guint16 compressedLength;
  guint16 compressionMethod;
  guint8 *buffer;

  if (read(resh, &(result->id),2) != 2)
    return SCI_ERROR_IO_ERROR;

#ifdef WORDS_BIGENDIAN
  result->id = GUINT16_SWAP_LE_BE_CONSTANT(result->id);
#endif

  result->number = result->id & 0x07ff;
  result->type = result->id >> 11;

  if ((result->number > 999) || (result->type > sci_invalid_resource))
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
  /* This return will never happen in SCI0 */

  if ((result->length > SCI_MAX_RESOURCE_SIZE) ||
      (compressedLength > SCI_MAX_RESOURCE_SIZE))
    return SCI_ERROR_RESOURCE_TOO_BIG;
  /* With SCI0, this simply cannot happen. */

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
  fprintf(stderr, "Resource %s.%03hi encrypted with method %hi at %.2f%%"
	  " ratio\n",
	  Resource_Types[result->type], result->number, compressionMethod,
	  (result->length == 0)? -1.0 : 
	  (100.0 * compressedLength / result->length));
  fprintf(stderr, "  compressedLength = 0x%hx, actualLength=0x%hx\n",
	  compressedLength, result->length);
#endif

  switch(compressionMethod) {

  case 0: /* no compression */
    memcpy(result->data, buffer, compressedLength);
    result->status = SCI_STATUS_OK;
    break;

  case 1: /* LZW compression */
    if (decrypt1(result->data, buffer, result->length, compressedLength)) {
      g_free(result->data);
      result->data = 0; /* So that we know that it didn't work */
      result->status = SCI_STATUS_NOMALLOC;
      g_free(buffer);
      return SCI_ERROR_DECOMPRESSION_OVERFLOW;
    }
    result->status = SCI_STATUS_OK;
    break;

  case 2: /* Some sort of Huffman encoding */
    if (decrypt2(result->data, buffer, result->length, compressedLength)) {
      g_free(result->data);
      result->data = 0; /* So that we know that it didn't work */
      result->status = SCI_STATUS_NOMALLOC;
      g_free(buffer);
      return SCI_ERROR_DECOMPRESSION_OVERFLOW;
    }
    result->status = SCI_STATUS_OK;
    break;

  default:
    fprintf(stderr,"Resource %s.%03hi: Compression method %hi not "
	    "supported!\n", Resource_Types[result->type], result->number,
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
