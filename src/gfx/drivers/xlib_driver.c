/***************************************************************************
 xlib_driver.h Copyright (C) 2000 Christoph Reichenbach


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

    Christoph Reichenbach (CR) <jameson@linuxgames.com>

***************************************************************************/

#include <gfx_driver.h>
#ifdef HAVE_X11_XLIB_H
#include <gfx_tools.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

struct _xlib_state {
	Display *display;
	Window window;
	GC gc;
	XGCValues gc_values;
	Colormap colormap;
	Pixmap visual[3];
	gfx_pixmap_t *priority[2];
	int buckystate;
	void *old_error_handler;
	Cursor mouse_cursor;
};

#define S ((struct _xlib_state *)(drv->state))

#define XASS(foo) { int val = foo; if (!val) xlderror(drv, __LINE__); }

#define DEBUGB if (drv->debug_flags & GFX_DEBUG_BASIC && ((debugline = __LINE__))) xldprintf
#define DEBUGU if (drv->debug_flags & GFX_DEBUG_UPDATES && ((debugline = __LINE__))) xldprintf
#define DEBUGPXM if (drv->debug_flags & GFX_DEBUG_PIXMAPS && ((debugline = __LINE__))) xldprintf
#define DEBUGPTR if (drv->debug_flags & GFX_DEBUG_POINTER && ((debugline = __LINE__))) xldprintf
#define ERROR if ((debugline = __LINE__)) xldprintf

static int debugline = 0;

static void
xldprintf(char *fmt, ...)
{
	va_list argp;

	fprintf(stderr,"GFX-XLIB %d:", debugline);
	va_start(argp, fmt);
	vfprintf(stderr, fmt, argp);
	va_end(argp);
}

static void
xlderror(gfx_driver_t *drv, int line)
{
	xldprintf("Xlib Error in line %d\n", line);
}

static unsigned long
xlib_map_color(gfx_driver_t *drv, gfx_color_t color)
{
	gfx_mode_t *mode = drv->mode;
	unsigned long temp;
	unsigned long retval = 0;

	if (drv->mode->palette)
		return color.visual.global_index;

	temp = color.visual.r;
	temp |= temp << 8;
	temp |= temp << 16;
	retval |= (temp >> mode->red_shift) & (mode->red_mask);
	temp = color.visual.g;
	temp |= temp << 8;
	temp |= temp << 16;
	retval |= (temp >> mode->green_shift) & (mode->green_mask);
	temp = color.visual.b;
	temp |= temp << 8;
	temp |= temp << 16;
	retval |= (temp >> mode->blue_shift) & (mode->blue_mask);

	return retval;
}

static unsigned long
xlib_map_pixmap_color(gfx_driver_t *drv, gfx_pixmap_color_t pc)
{
	gfx_color_t color;

	color.mask = GFX_MASK_VISUAL;
	memcpy(&(color.visual), &pc, sizeof(gfx_pixmap_color_t));

	return xlib_map_color(drv, color);
}


static int
xlib_error_handler(Display *display, XErrorEvent *error)
{
	char errormsg[256];
	XGetErrorText(display, error->error_code, errormsg, 255);
	ERROR("X11 error: %s\n", errormsg);

	return 0;
}


static int
xlib_set_parameter(struct _gfx_driver *drv, char *attribute, char *value)
{
	ERROR("Attempt to set xlib parameter \"%s\" to \"%s\"\n", attribute, value);
	return GFX_ERROR;
}

Cursor
x_empty_cursor(Display *display, Drawable drawable) /* Generates an empty X cursor */
{
	byte cursor_data[] = {0};
	XColor black = {0,0,0};
	Pixmap cursor_map;

	Cursor retval;

	cursor_map = XCreateBitmapFromData(display, drawable, cursor_data, 1, 1);

	retval = XCreatePixmapCursor(display, cursor_map, cursor_map, &black, &black, 0, 0);

	XFreePixmap(display, cursor_map);

	return retval;
}


static int
xlib_init_specific(struct _gfx_driver *drv, int xfact, int yfact, int bytespp)
{
	XVisualInfo xvisinfo;
	XSetWindowAttributes win_attr;
	int default_screen, num_aux_buffers;
	int vistype = (bytespp == 1)? 3 /* PseudoColor */ : 4 /* TrueColor */;
	int found_vistype = 0;
	int red_shift, green_shift, blue_shift;
	int xsize = xfact * 320;
	int ysize = yfact * 200;
	XSizeHints *size_hints;
	int i;

	if (!S)
		S = malloc(sizeof(struct _xlib_state));

	if (xfact < 1 || yfact < 1 || bytespp < 1 || bytespp > 4) {
		ERROR("Internal error: Attempt to open window w/ scale factors (%d,%d) and bpp=%d!\n",
		      xfact, yfact, bytespp);
	}

	S->display = XOpenDisplay(NULL);

	if (!S->display) {
		ERROR("Could not open X connection!\n");
		return GFX_FATAL;
	}

	default_screen = DefaultScreen(S->display);

	while ((((bytespp > 1) && (vistype >= 4))
		|| ((bytespp == 1) && (vistype == 3)))
	       && !XMatchVisualInfo(S->display, default_screen, bytespp << 3, vistype, &xvisinfo)) {
		vistype--;
	}

	if (vistype < 3 || ((vistype == 3) && (bytespp != 1))) {
		if (bytespp == 1) {
			ERROR("Could not get an 8 bit Pseudocolor visual!\n");
		} else {
			ERROR("Could not get a %d bit TrueColor visual!\n", bytespp << 3);
		}
		return GFX_FATAL;
	}

	S->colormap = win_attr.colormap =
		XCreateColormap(S->display, RootWindow(S->display, default_screen),
				xvisinfo.visual, AllocNone);

	win_attr.event_mask = PointerMotionMask | StructureNotifyMask | ButtonPressMask
		| ButtonReleaseMask | KeyPressMask | KeyReleaseMask;
	win_attr.background_pixel = win_attr.border_pixel = 0;

	S->window = XCreateWindow(S->display, RootWindow(S->display, default_screen),
				  0, 0, xsize, ysize, 0, xvisinfo.depth, InputOutput,
				  xvisinfo.visual, (CWBackPixel | CWBorderPixel | CWColormap | CWEventMask),
				  &win_attr);

	if (!S->window) {
		ERROR("Could not create window of size %dx%d!\n", 320*xfact, 200*yfact);
		return GFX_FATAL;
	}

	XSync(S->display, False);

	XStoreName(S->display, S->window, "FreeSCI");
	XDefineCursor(S->display, S->window, (S->mouse_cursor = x_empty_cursor(S->display, S->window)));

	XMapWindow(S->display, S->window);
	S->buckystate = 0;

	if (bytespp == 1)
		red_shift = green_shift = blue_shift = 0;
	else {
		red_shift = 32 - ffs((~xvisinfo.red_mask) & (xvisinfo.red_mask << 1));
		green_shift = 32 - ffs((~xvisinfo.green_mask) & (xvisinfo.green_mask << 1));
		blue_shift = 32 - ffs((~xvisinfo.blue_mask) & (xvisinfo.blue_mask << 1));
	}

	drv->mode = gfx_new_mode(xfact, yfact, bytespp,
				 xvisinfo.red_mask, xvisinfo.green_mask, xvisinfo.blue_mask,
				 0, /* alpha mask */
				 red_shift, green_shift, blue_shift,
				 0, /* alpha shift */
				 (bytespp == 1)? xvisinfo.colormap_size : 0);

	size_hints = XAllocSizeHints();
	size_hints->base_width = size_hints->min_width = size_hints->max_width = xsize;
	size_hints->base_height = size_hints->min_height = size_hints->max_height = ysize;
	size_hints->flags |= PMinSize | PMaxSize | PBaseSize;
	XSetWMNormalHints(S->display, S->window, size_hints);
	XFree(size_hints);

	S->gc_values.foreground = BlackPixel(S->display, DefaultScreen(S->display));
	S->gc = XCreateGC(S->display, S->window, GCForeground, &(S->gc_values));

	for (i = 0; i < 2; i++) {
		S->priority[i] = gfx_pixmap_alloc_index_data(gfx_new_pixmap(xsize, ysize, GFX_RESID_NONE, -i, -777));
		if (!S->priority[i]) {
			ERROR("Out of memory: Could not allocate priority maps! (%dx%d)\n",
			      xsize, ysize);
			return GFX_FATAL;
		}
	}

	for (i = 0; i < 3; i++) {
		S->visual[i] = XCreatePixmap(S->display, S->window, xsize, ysize, bytespp << 3);
		XFillRectangle(S->display, S->visual[i], S->gc, 0, 0, xsize, ysize);
	}

	S->old_error_handler = XSetErrorHandler(xlib_error_handler);

	return GFX_OK;
}



static int
xlib_init(struct _gfx_driver *drv)
{
	int i;
	for (i = 1; i <= 4; i++)
		if (!xlib_init_specific(drv, 2, 2, i))
			return GFX_OK;

	DEBUGB("Failed to find visual!\n");
    

	return GFX_FATAL;
}

static void
xlib_exit(struct _gfx_driver *drv)
{
	int i;
	if (S) {
		for (i = 0; i < 2; i++) {
			gfx_free_pixmap(drv, S->priority[i]);
			S->priority[i] = NULL;
		}

		for (i = 0; i < 3; i++)
			XFreePixmap(S->display, S->visual[i]);

		XFreeGC(S->display, S->gc);
		XDestroyWindow(S->display, S->window);
		XCloseDisplay(S->display);
		XSetErrorHandler(S->old_error_handler);
		free(S);
		S = NULL;
	}
}


  /*** Drawing operations ***/

static int
xlib_draw_line(struct _gfx_driver *drv, rect_t line, gfx_color_t color,
               gfx_line_mode_t line_mode, gfx_line_style_t line_style)
{
	int linewidth = (line_mode == GFX_LINE_MODE_FINE)? 1:
		(drv->mode->xfact + drv->mode->yfact) >> 1;

	if (color.mask & GFX_MASK_VISUAL) {
		S->gc_values.foreground = xlib_map_color(drv, color);
		S->gc_values.line_width = linewidth;
		S->gc_values.line_style = (line_style == GFX_LINE_STYLE_NORMAL)?
			LineSolid : LineOnOffDash;

		XChangeGC(S->display, S->gc, GCLineWidth | GCLineStyle | GCForeground, &(S->gc_values));
		XASS(XDrawLine(S->display, S->visual[1], S->gc, line.x, line.y,
			       line.x + line.xl, line.y + line.yl));
	}

	if (color.mask & GFX_MASK_PRIORITY) {
		int xc, yc;
		rect_t newline;

		newline.xl = line.xl;
		newline.yl = line.yl;

		linewidth--;
		for (xc = -linewidth; xc++; xc <= linewidth)
			for (yc = -linewidth; xc++; xc <= linewidth) {
				newline.x = line.x + xc;
				newline.y = line.y + yc;
				gfx_draw_line_pixmap_i(S->priority[0], newline, color.priority);
			}
	}

	return GFX_OK;
}

static int
xlib_draw_filled_rect(struct _gfx_driver *drv, rect_t rect,
                      gfx_color_t color1, gfx_color_t color2,
                      gfx_rectangle_fill_t shade_mode)
{

	if (color1.mask & GFX_MASK_VISUAL) {
		S->gc_values.foreground = xlib_map_color(drv, color1);
		XChangeGC(S->display, S->gc, GCForeground, &(S->gc_values));
		XASS(XFillRectangle(S->display, S->visual[1], S->gc, rect.x, rect.y,
				    rect.xl, rect.yl));
	}

	if (color1.mask & GFX_MASK_PRIORITY)
		gfx_draw_box_pixmap_i(S->priority[0], rect, color1.priority);

	return GFX_OK;
}

  /*** Pixmap operations ***/

static int
xlib_register_pixmap(struct _gfx_driver *drv, gfx_pixmap_t *pxm)
{
	if (pxm->internal.info) {
		ERROR("Attempt to register pixmap twice!\n");
		return GFX_ERROR;
	}

	pxm->internal.info = XCreateImage(S->display, DefaultVisual(S->display, DefaultScreen(S->display)),
					  drv->mode->bytespp << 3, XYPixmap, 0, pxm->data, pxm->xl,
					  pxm->yl, 8, 0);
	return GFX_OK;
}

static int
xlib_unregister_pixmap(struct _gfx_driver *drv, gfx_pixmap_t *pxm)
{
	if (!pxm->internal.info) {
		ERROR("Attempt to unregister pixmap twice!\n");
		return GFX_ERROR;
	}

	XDestroyImage((XImage *) pxm->internal.info);
	pxm->internal.info = NULL;
	return GFX_OK;
}

static int
xlib_draw_pixmap(struct _gfx_driver *drv, gfx_pixmap_t *pxm, int priority,
                 rect_t src, rect_t dest, gfx_buffer_t buffer)
  /* Draws part of a pixmap to the static or back buffer
  ** Parameters: (gfx_driver_t *) drv: The affected driver
  **             (gfx_pixmap_t *) pxm: The pixmap to draw
  **             (int) priority: The priority to draw with, or GFX_NO_PRIORITY
  **                   to draw on top of everything without setting the
  **                   priority back buffer
  **             (rect_t) src: The pixmap-relative source rectangle
  **             (rect_t) dest: The destination rectangle
  **             (int) buffer: One of GFX_BUFFER_STATIC and GFX_BUFFER_BACK
  ** Returns   : (int) GFX_OK or GFX_FATAL, or GFX_ERROR if pxm was not
  **                   (but should have been) registered.
  ** dest.xl and dest.yl must be evaluated and used for scaling if
  ** GFX_CAPABILITY_SCALEABLE_PIXMAPS is supported.
  */
{
}

static int
xlib_grab_pixmap(struct _gfx_driver *drv, rect_t src, gfx_pixmap_t *pxm,
                 gfx_map_mask_t map)
  /* Grabs an image from the visual or priority back buffer
  ** Parameters: (gfx_driver_t *) drv: The affected driver
  **             (rect_t) src: The rectangle to grab
  **             (gfx_pixmap_t *) pxm: The pixmap structure the data is to
  **                              be written to
  **             (int) map: GFX_MASK_VISUAL or GFX_MASK_PRIORITY
  ** Returns   : (int) GFX_OK, GFX_FATAL, or GFX_ERROR for invalid map values
  ** pxm may be assumed to be empty and pre-allocated with an appropriate
  ** memory size.
  ** This function is optional if GFX_CAPABILITY_PIXMAP_GRABBING is not set.
  */
{
}


  /*** Buffer operations ***/

static int
xlib_update(struct _gfx_driver *drv, rect_t src, point_t dest, gfx_buffer_t buffer)
  /* Updates the front buffer or the back buffers
  ** Parameters: (gfx_driver_t *) drv: The affected driver
  **             (rect_t) src: Source rectangle
  **             (point_t) dest: Destination point
  **             (int) buffer: One of GFX_BUFFER_FRONT and GFX_BUFFER_BACK
  ** Returns   : (int) GFX_OK, GFX_ERROR or GFX_FATAL
  ** This function updates either the visual front buffer, or the two back
  ** buffers, by copying the specified source region to the destination
  ** region.
  ** For heuristical reasons, it may be assumed that the x and y fields of
  ** src and dest will be identical in /most/ cases.
  ** If they aren't, the priority map will not be required to be copied.
  */
{
	int data_source = (buffer == GFX_BUFFER_BACK)? 2 : 1;
	int data_dest = data_source - 1;

	XCopyArea(S->display, S->visual[data_source], S->visual[data_dest], S->gc,
		  src.x, src.y, src.xl, src.yl, dest.x, dest.y);

	if (buffer == GFX_BUFFER_BACK)
		gfx_copy_pixmap_box_i(S->priority[0], S->priority[1], src);
	else
		XCopyArea(S->display, S->visual[0], S->window, S->gc,
			  src.x, src.y, src.xl, src.yl, dest.x, dest.y);

	return GFX_OK;
}

static int
xlib_set_static_buffer(struct _gfx_driver *drv, gfx_pixmap_t *pic, gfx_pixmap_t *priority)
  /* Sets the contents of the static visual and priority buffers
  ** Parameters: (gfx_driver_t *) drv: The affected driver
  **             (gfx_pixmap_t *) pic: The image defining the new content
  **                              of the visual back buffer
  **             (gfx_pixmap_t *) priority: The priority map containing
  **                              the new content of the priority back buffer
  **                              in the index buffer
  ** Returns   : (int) GFX_OK or GFX_FATAL
  ** pic and priority may be modified or written to freely. They may also be
  ** used as the actual static buffers, since they are not freed and re-
  ** allocated between calls to set_static_buffer() and update(), unless
  ** exit() was called in between.
  ** Note that later version of the driver interface may disallow modifying
  ** pic and priority.
  ** pic and priority are always scaled to the appropriate resolution, even
  ** if GFX_CAPABILITY_SCALEABLE_PIXMAPS is set.
  */
{
}


  /*** Mouse pointer operations ***/

static byte *
xlib_create_cursor_data(gfx_driver_t *drv, gfx_pixmap_t *pointer, int mode)
{
	int linewidth = (pointer->xl + 7) >> 3;
	int lines = pointer->yl;
	int xc, yc;
	int xfact = drv->mode->xfact;
	byte *data = calloc(linewidth, lines);
	byte *linebase = data, *pos;
	byte *src = pointer->index_data;


	if (pointer->index_xl & 7) {
		ERROR("No support for non-multiples of 8 in cursor generation!\n");
		return NULL;
	}

	for (yc = 0; yc < pointer->index_yl; yc++) {
		int scalectr;
		int bitc = 0;
		pos = linebase;


		for (xc = 0; xc < pointer->index_xl; xc++) {
			int draw = mode?
				(*src == 0) : (*src < 255);

			for (scalectr = 0; scalectr < xfact; scalectr++) {
				if (draw)
					*pos |= (1 << bitc);

				bitc++;
				if (bitc == 8) {
					bitc = 0;
					pos++;
				}
			}

			src++;
		}
		for (scalectr = 1; scalectr < drv->mode->yfact; scalectr++)
			memcpy(linebase + linewidth * scalectr, linebase, linewidth);

		linebase += linewidth * drv->mode->yfact;
	}

	return data;
}

static int
xlib_set_pointer(struct _gfx_driver *drv, gfx_pixmap_t *pointer)
  /* Sets a new mouse pointer.
  ** Parameters: (gfx_driver_t *) drv: The driver to modify
  **             (gfx_pixmap_t *) pointer: The pointer to set, or NULL to set
  **                              no pointer
  ** Returns   : (int) GFX_OK or GFX_FATAL
  ** This function may be NULL if GFX_CAPABILITY_MOUSE_POINTER is not set.
  ** If pointer is not NULL, it will have been scaled to the appropriate
  ** size and registered as a pixmap (if neccessary) beforehand.
  ** If this function is called for a target that supports only two-color
  ** pointers, the image is a color index image, where only color index values
  ** 0, 1, and GFX_COLOR_INDEX_TRANSPARENT are used.
  */
{
	XFreeCursor(S->display, S->mouse_cursor);

	if (pointer == NULL)
		S->mouse_cursor = x_empty_cursor(S->display, S->window);
	else {
		XColor cols[2];
		Pixmap visual, mask;
		byte *mask_data, *visual_data;
		int i, j;

		for (i = 0; i < 2; i++) {
			//			cols[i].pixel = xlib_map_pixmap_color(drv, pointer->colors[i]);
			cols[i].red = pointer->colors[i].r;
			cols[i].red |= (cols[i].red << 8);
			cols[i].green = pointer->colors[i].g;
			cols[i].green |= (cols[i].green << 8);
			cols[i].blue = pointer->colors[i].b;
			cols[i].blue |= (cols[i].blue << 8);
		}

		visual_data = xlib_create_cursor_data(drv, pointer, 1);
		mask_data = xlib_create_cursor_data(drv, pointer, 0);
		visual = XCreateBitmapFromData(S->display, S->window, visual_data, pointer->xl, pointer->yl);
		mask = XCreateBitmapFromData(S->display, S->window, mask_data, pointer->xl, pointer->yl);
		

		S->mouse_cursor =
			XCreatePixmapCursor(S->display, visual, mask,
					    &(cols[0]), &(cols[1]),
					    pointer->xoffset, pointer->yoffset);

		XFreePixmap(S->display, visual);
		XFreePixmap(S->display, mask);
		free(mask_data);
	}

	XDefineCursor(S->display, S->window, S->mouse_cursor);
}


  /*** Palette operations ***/

static int
xlib_set_palette(struct _gfx_driver *drv, int index, byte red, byte green, byte blue)
  /* Manipulates a palette index in the hardware palette
  ** Parameters: (gfx_driver_t *) drv: The driver affected
  **             (int) index: The index of the palette entry to modify
  **             (int x int x int) red, green, blue: The RGB intensities to
  **                               set for the specified index. The minimum
  **                               intensity is 0, maximum is 0xff.
  ** Returns   : (int) GFX_OK, GFX_ERROR or GFX_FATAL
  ** This function does not need to update mode->palette, as this is done
  ** by the calling code.
  ** set_palette() is only required for targets supporting color index mode.
  */
{
}


  /*** Event management ***/

int
x_unmap_key(gfx_driver_t *drv, int keycode)
{
	KeySym xkey = XKeycodeToKeysym(S->display, keycode, 0);

	switch (xkey) {
	case XK_Control_L:
	case XK_Control_R: S->buckystate &= ~SCI_EVM_CTRL; return 0;
	case XK_Alt_L:
	case XK_Alt_R: S->buckystate &= ~SCI_EVM_ALT; return 0;
	case XK_Shift_L: S->buckystate &= ~SCI_EVM_LSHIFT; return 0;
	case XK_Shift_R: S->buckystate &= ~SCI_EVM_RSHIFT; return 0;
	}


	return 0;
}


int
x_map_key(gfx_driver_t *drv, int keycode)
{
	KeySym xkey = XKeycodeToKeysym(S->display, keycode, 0);

	if ((xkey >= 'A') && (xkey <= 'Z'))
		return xkey;
	if ((xkey >= 'a') && (xkey <= 'z'))
		return xkey;
	if ((xkey >= '0') && (xkey <= '9'))
		return xkey;

	switch (xkey) {
	case XK_BackSpace: return SCI_K_BACKSPACE;
	case XK_Tab: return 9;
	case XK_Escape: return SCI_K_ESC;
	case XK_Return:
	case XK_KP_Enter: return SCI_K_ENTER;

	case XK_KP_Decimal: return SCI_K_DELETE;
	case XK_KP_0:
	case XK_KP_Insert: return SCI_K_INSERT;
	case XK_KP_End:
	case XK_KP_1: return SCI_K_END;
	case XK_Down:
	case XK_KP_Down:
	case XK_KP_2: return SCI_K_DOWN;
	case XK_KP_Page_Down:
	case XK_KP_3: return SCI_K_PGDOWN;
	case XK_Left:
	case XK_KP_Left:
	case XK_KP_4: return SCI_K_LEFT;
	case XK_KP_5: return SCI_K_CENTER;
	case XK_Right:
	case XK_KP_Right:
	case XK_KP_6: return SCI_K_RIGHT;
	case XK_KP_Home:
	case XK_KP_7: return SCI_K_HOME;
	case XK_Up:
	case XK_KP_Up:
	case XK_KP_8: return SCI_K_UP;
	case XK_KP_Page_Up:
	case XK_KP_9: return SCI_K_PGUP;

	case XK_F1: return SCI_K_F1;
	case XK_F2: return SCI_K_F2;
	case XK_F3: return SCI_K_F3;
	case XK_F4: return SCI_K_F4;
	case XK_F5: return SCI_K_F5;
	case XK_F6: return SCI_K_F6;
	case XK_F7: return SCI_K_F7;
	case XK_F8: return SCI_K_F8;
	case XK_F9: return SCI_K_F9;
	case XK_F10: return SCI_K_F10;

	case XK_Control_L:
	case XK_Control_R: S->buckystate |= SCI_EVM_CTRL; return 0;
	case XK_Alt_L:
	case XK_Alt_R: S->buckystate |= SCI_EVM_ALT; return 0;
	case XK_Caps_Lock:
	case XK_Shift_Lock: S->buckystate ^= SCI_EVM_CAPSLOCK; return 0;
	case XK_Scroll_Lock: S->buckystate ^= SCI_EVM_SCRLOCK; return 0;
	case XK_Num_Lock: S->buckystate ^= SCI_EVM_NUMLOCK; return 0;
	case XK_Shift_L: S->buckystate |= SCI_EVM_LSHIFT; return 0;
	case XK_Shift_R: S->buckystate |= SCI_EVM_RSHIFT; return 0;

	case XK_KP_Add: return '+';
	case XK_KP_Divide: return '/';
	case XK_KP_Subtract: return '-';
	case XK_KP_Multiply: return '*';

	case ',':
	case '.':
	case '/':
	case '\\':
	case ';':
	case '\'':
	case '[':
	case ']':
	case '`':
	case '-':
	case '=':
	case '<':
	case ' ':
		return xkey;
	}

	sciprintf("Unknown X keysym: %04x\n", xkey);
	return 0;
}


void
x_get_event(gfx_driver_t *drv, int eventmask, long wait_usec, sci_event_t *sci_event)
{
	XEvent event;
	Window window = S->window;
	Display *display = S->display;
	struct timeval ctime, timeout_time, sleep_time;
	int usecs_to_sleep;

	gettimeofday(&timeout_time, NULL);
	timeout_time.tv_usec += wait_usec;

	/* Calculate wait time */
	timeout_time.tv_sec += (timeout_time.tv_usec / 1000000);
	timeout_time.tv_usec %= 1000000;

	do {
		int redraw_pointer_request = 0;

		while (XCheckWindowEvent(display, window, eventmask, &event)) {
			switch (event.type) {

			case KeyPress: {
				sci_event->type = SCI_EVT_KEYBOARD;
				sci_event->buckybits = S->buckystate;
				sci_event->data = x_map_key(drv, event.xkey.keycode);

				if (sci_event->data)
					return;

				break;
			}

			case KeyRelease:
				x_unmap_key(drv, event.xkey.keycode);
				break;

			case ButtonPress: {
				sci_event->type = SCI_EVT_MOUSE_PRESS;
				sci_event->buckybits = event.xkey.state;
				sci_event->data = 0;
				return;
			}

			case ButtonRelease: {
				sci_event->type = SCI_EVT_MOUSE_RELEASE;
				sci_event->buckybits = event.xkey.state;
				sci_event->data = 0;
				return;
			}

			case MotionNotify: {

				drv->pointer_x = event.xmotion.x;
				drv->pointer_y = event.xmotion.y;
			}
			break;

			default:
				ERROR("Received unhandled X event %04x\n", event.type);
			}
		}

		gettimeofday(&ctime, NULL);

		usecs_to_sleep = (timeout_time.tv_sec > ctime.tv_sec)? 1000000 : 0;
		usecs_to_sleep += timeout_time.tv_usec - ctime.tv_usec;
		if (ctime.tv_sec > timeout_time.tv_sec) usecs_to_sleep = -1;


		if (usecs_to_sleep > 0) {

			if (usecs_to_sleep > 10000)
				usecs_to_sleep = 10000; /* Sleep for a maximum of 10 ms */

			sleep_time.tv_usec = usecs_to_sleep;
			sleep_time.tv_sec = 0;

			select(0, NULL, NULL, NULL, &sleep_time); /* Sleep. */
		}

	} while (usecs_to_sleep >= 0);

	if (sci_event)
		sci_event->type = SCI_EVT_NONE; /* No event. */
}


static sci_event_t
xlib_get_event(struct _gfx_driver *drv)
{
	sci_event_t input;

	x_get_event(drv, PointerMotionMask | StructureNotifyMask | ButtonPressMask
		    | ButtonReleaseMask | KeyPressMask | KeyReleaseMask,
		    0, &input);

	return input;
}


static int
xlib_usec_sleep(struct _gfx_driver *drv, int usecs)
{
	x_get_event(drv, PointerMotionMask | StructureNotifyMask, usecs, NULL);
	return GFX_OK;
}

gfx_driver_t
gfx_driver_xlib = {
	"xlib",
	"0.1",
	NULL,
	0, 0,
	GFX_CAPABILITY_STIPPLED_LINES | GFX_CAPABILITY_MOUSE_SUPPORT | GFX_CAPABILITY_MOUSE_POINTER,
	0,
	xlib_set_parameter,
	xlib_init_specific,
	xlib_init,
	xlib_exit,
	xlib_draw_line,
	xlib_draw_filled_rect,
	xlib_register_pixmap,
	xlib_unregister_pixmap,
	xlib_draw_pixmap,
	xlib_grab_pixmap,
	xlib_update,
	xlib_set_static_buffer,
	xlib_set_pointer,
	xlib_set_palette,
	xlib_get_event,
	xlib_usec_sleep,
	NULL
};

#endif /* HAVE_XLIB */