/***************************************************************************
 graphics.h (C) 1999 Christoph Reichenbach, TU Darmstadt


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

   990401 - created (honestly!) (CJR)

***************************************************************************/

#ifndef _SCI_GRAPHICS_H_
#define _SCI_GRAPHICS_H_

#include <resource.h>
#include <vm.h>


#define SCI_GRAPHICS_ALLOW_256
/* Allows 256 color modes */

#define _DEBUG_VISUALS
/* Use this define only if multiple visuals are available for your
** target, e.g. under X.
*/

typedef guint8** picture_t;
/* Used for storing "picture" resources (the background images). These
** have four layers: the actual screen buffer, a priority buffer (essentially
** a simple z buffer), a 'special' buffer (defining, among other things,
** the walkable area), and a write buffer, which is used internally while
** creating the picture.
*/


typedef struct {
  guint16 padding; /* Used for compatibility to window_t */

  gint16 ymin, xmin; /* Upper left corner */
  gint16 ymax, xmax; /* Lower right corner */
} port_t;


typedef struct {
  guint16 heapsize; /* SCI heap block size. Just leave it alone. */

  gint16 ymin, xmin; /* Upper left corner */
  gint16 ymax, xmax; /* Lower right corner */

  heap_ptr title; /* Window title (if applicable) */

  gint16 flags; /* Window flags. See below. */

  gint16 priority, bgcolor, color; /* Priority/color values as usual */
} window_t; /* Can be typecast safely to become a port_t */


#define SCI_COLOR_DITHER 0
/* Standard mode */
#define SCI_COLOR_INTERPOLATE 1
/* Interpolate colors */
#define SCI_COLOR_DITHER256 2
/* Dither with 256 colors */


#define SCI_FILL_NORMAL 1
/* Fill with the selected color */
#define SCI_FILL_BLACK 0
/* Fill with black */



/* The following flags are applicable to windows in SCI0: */
#define WINDOW_FLAG_TRANSPARENT 0x01
/* Window doesn't get filled with background color */

#define WINDOW_FLAG_NOFRAME 0x02
/* No frame is drawn around the window onto wm_view */

#define WINDOW_FLAG_TITLE 0x04
/* Add titlebar to window (10 pixels high, framed, text is centered and written
** in white on dark gray
*/

#define WINDOW_FLAG_DONTDRAW 0x80
/* Don't draw anything */


extern int sci_color_mode;
/* sci_color_interpolate forces 16 color background pictures to be drawn
** with 256 interpolated colors instead of 16 dithered colors
*/


/*** FUNCTION DECLARATIONS ***/

picture_t allocEmptyPicture();
/* Creates an initialized but empty picture buffer.
** Paramters: void
** Returns  : (picture_t) An empty picture.
*/

void freePicture(picture_t picture);
/* Deallocates all memory associated by the specified picture_t.
** Parameters: (picture_t) picture: The picture to deallocate.
** Returns   : (void)
*/

void copyPicture(picture_t dest, picture_t src);
/* Copies the content of a picture.
** Parameters: (picture_t) dest: An INITIALIZED picture which the data is to
**                                be copied to.
**             (picture_t) src: The picture which should be duplicated.
** Returns   : (void)
*/

void drawPicture0(picture_t dest, int flags, int defaultPalette, guint8* data);
/* Draws a picture resource to a picture_t buffer.
** Parameters: (picture_t) dest: The initialized picture buffer to draw to.
**             (int) flags: The picture flags. Currently, only bit 0 is used;
**                          with bit0, pictures are filled normally, with !bit0,
**                          fill commands are executed by filling in black.
**             (int) defaultPalette: The default palette to use for drawing
**                          (used to distinguish between day and night in some
**                          games)
**             (guint8*) data: The data to draw (usually resource_t.data).
** Remember that this function is much slower than copyPicture, so you should
** store a backup copy of the drawn picture if you want to use it to display
** some animation.
*/

void clearPicture(picture_t pic, int fgcol);
/* Clears a picture
** Parameters: (picture_t) pic: The picture to clear
**             (int) fgcol: The foreground color to use for screen 0
** Returns   : (void)
** Clearing a picture means that buffers 1-3 are set to 0, while buffer 0
** is set to zero for the first ten pixel rows and to 15 for the remainder.
** This must be called before a drawView0(), unless the picture is intended
** to be drawn ontop of an already existing picture.
*/

int drawView0(picture_t dest, port_t *port, int x, int y, short priority,
	      short group, short index, guint8 *data);
/* Draws a specified element of a view resource to a picture.
** Parameters: (picture_t) dest: The picture_t to draw to.
**             (port_t *) port: The viewport to draw to (NULL for the WM port)
**             (int) x,y: The position to draw to (clipping is performed).
**             (short) priority: The image's priority in the picture. Images
**                               with a higher priority cannot be overdrawn
**                               by pictures with a lower priority. Sensible
**                               values range from 0 to 15.
**             (short) group: A view resource consists of one or more groups of
**                            images, which usually form an animation. Use this
**                            value to determine the group.
**             (short) index: The picture index inside the specified group.
**             (guint8*) data: The data to draw (usually resource_t.data).
** Returns   : (int) 0 on success, -1 if the specified group could not be
**             found, or -2 if the index inside the group is invalid.
*/

void drawBox(picture_t dest, short x, short y, short xl, short yl, char color, char priority);
/* Draws a simple box.
** Parameters: (picture_t) dest: The picture_t to draw to.
**             (short) x,y: The coordinates to draw to.
**             (short) xl,yl: The width and height of the box.
**             (char) color: The color to draw with.
**             (char) priority: The priority to fill the box with (it still overwrites anything)
** Returns   : (void)
** The box does not come with any fancy shading. Use drawWindow to do this.
*/


void drawWindow(picture_t dest, port_t *port, char color, char priority,
		char *title, guint8 *titlefont, gint16 flags);
/* Draws a window with the appropriate facilities
** Parameters: (picture_t) dest: The picture_t to draw to
**             (port_t *) port: The port to draw to
**             (short) color: The background color of the window
**             (short) priority: The window's priority
**             (char *) title: The title to draw (see flags)
**             (char *) titlefont: The font which the title should be drawn with
**             (gint16) flags: The window flags (see the beginning of this header file)
** This function will draw a window; it is very similar to the kernel call NewWindow() in its
** functionality.
*/


void drawText0(picture_t dest, port_t *port, int x, int y, char *text, char *font, char color);
void drawTextCentered0(picture_t dest, port_t *port, int x, int y, char *text, char *font, char color);
/* Draws text in a specific font and color.
** Parameters: (picture_t) dest: The picture_t to draw to.
**             (port_t *) port: The port to draw to.
**             (int) x, y: The coordinates (relative to the port) to draw to.
**             (char *) text: The text to draw.
**             (char *) font: The font to draw the text in.
**             (char) color: The color to use for drawing.
** Returns   : (void)
** This will only draw the supplied text, without any surrounding box.
** drawTextCentered will, in addition, center the text to the supplied port.
*/


void drawMouseCursor(picture_t target, int x, int y, guint8 *cursor);
/* Draws a mouse cursor
** Parameters: target: The picture_t to draw to
**             (x,y): The coordinates to draw to (are clipped to valid values)
**             cursor: The cursor data to draw
** This function currently uses SCI0 cursor drawing for everything.
*/

#endif
