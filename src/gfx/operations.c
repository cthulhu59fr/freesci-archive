/***************************************************************************
 gfx_operations Copyright (C) 2000 Christoph Reichenbach


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
/* Graphical operations, called from the widget state manager */


#include <gfx_operations.h>

#define POINTER_VISIBLE_BUT_CLIPPED 2

/* Performs basic checks that apply to most functions */
#define BASIC_CHECKS(error_retval) \
if (!state) { \
	GFXERROR("Null state!\n"); \
	return error_retval; \
} \
if (!state->driver) { \
	GFXERROR("GFX driver invalid!\n"); \
	return error_retval; \
}

/* How to determine whether colors have to be allocated */
#define PALETTE_MODE state->driver->mode->palette

#define DRAW_POINTER { int __x = _gfxop_draw_pointer(state); if (__x) { GFXERROR("Drawing the mouse pointer failed!\n"); return __x;} }
#define REMOVE_POINTER { int __x = _gfxop_remove_pointer(state); if (__x) { GFXERROR("Removing the mouse pointer failed!\n"); return __x;} }


/* Internal operations */

static void
_gfxop_scale_rect(rect_t *rect, gfx_mode_t *mode)
{
	int xfact = mode->xfact;
	int yfact = mode->yfact;

	rect->x *= xfact;
	rect->y *= yfact;
	rect->xl *= xfact;
	rect->yl *= yfact;
}

static void
_gfxop_alloc_colors(gfx_state_t *state, gfx_pixmap_color_t *colors, int colors_nr)
{
	int i;

	if (!PALETTE_MODE)
		return;
	
	for (i = 0; i < colors_nr; i++)
		gfx_alloc_color(state->driver->mode->palette, colors + i); 
}

static void
_gfxop_free_colors(gfx_state_t *state, gfx_pixmap_color_t *colors, int colors_nr)
{
	int i;

	if (!PALETTE_MODE)
		return;
	
	for (i = 0; i < colors_nr; i++)
		gfx_free_color(state->driver->mode->palette, colors + i); 
}


int _gfxop_clip(rect_t *rect, rect_t clipzone)
/* Returns 1 if nothing is left */
{
	if (rect->x < clipzone.x) {
		rect->xl -= (clipzone.x - rect->x);
		rect->x = clipzone.x;
	}

	if (rect->y < clipzone.y) {
		rect->yl -= (clipzone.y - rect->y);
		rect->y = clipzone.y;
	}

	if (rect->x + rect->xl > clipzone.x + clipzone.xl)
		rect->xl = (clipzone.x + clipzone.xl) - rect->x;

	if (rect->y + rect->yl > clipzone.y + clipzone.yl)
		rect->yl = (clipzone.y + clipzone.yl) - rect->y;

	return (rect->xl <= 0 || rect->yl <= 0);
}

static int
_gfxop_grab_pixmap(gfx_state_t *state, gfx_pixmap_t **pxmp, int x, int y,
		   int xl, int yl, int priority, rect_t *zone)
     /* Returns 1 if the resulting data size was zero, GFX_OK or an error code otherwise */
{
	int xfact = state->driver->mode->xfact;
	int yfact = state->driver->mode->yfact;
	int unscaled_xl = (xl + xfact - 1) / xfact;
	int unscaled_yl = (yl + yfact - 1) / yfact;
	*zone = gfx_rect(x, y, xl, yl);

	if (!(state->driver->capabilities & GFX_CAPABILITY_PIXMAP_GRABBING)) {
		GFXERROR("Attempt to grab pixmap even though driver does not support pixmap grabbing!");
		return GFX_FATAL;
	}

	if (_gfxop_clip(zone, gfx_rect(0, 0,
				       320 * state->driver->mode->xfact,
				       200 * state->driver->mode->yfact)))
		return GFX_ERROR;

	if (!*pxmp)
		*pxmp = gfx_new_pixmap(unscaled_xl, unscaled_yl, GFX_RESID_NONE, 0, 0);
	else
		if (xl * yl > (*pxmp)->xl * (*pxmp)->yl) {
			gfx_pixmap_free_data(*pxmp);
			(*pxmp)->data = NULL;
		}

	if (!(*pxmp)->data) {
		(*pxmp)->index_xl = unscaled_xl + 1;
		(*pxmp)->index_yl = unscaled_yl + 1;
		gfx_pixmap_alloc_data(*pxmp, state->driver->mode);
	}

	return state->driver->grab_pixmap(state->driver, *zone, *pxmp,
					  priority? GFX_MASK_PRIORITY : GFX_MASK_VISUAL);
}

static void
_gfxop_draw_control(gfx_pixmap_t *map, gfx_pixmap_t *pxm, int color, point_t pos)
{
	rect_t drawrect = gfx_rect(pos.x, pos.y, pxm->index_xl, pxm->index_yl);
	int offset, base_offset;
	int read_offset, base_read_offset;
	int x,y;

	if (!pxm->index_data) {
		GFXERROR("Attempt to draw control color %d on pixmap %d/%d/%d without index data!\n",
			 color, pxm->ID, pxm->loop, pxm->cel);
		return;
	}

	if (_gfxop_clip(&drawrect, gfx_rect(0, 0, 320, 200)))
		return;

	offset = base_offset = drawrect.x + drawrect.y * 320;
	read_offset = base_read_offset = (drawrect.x - pos.x) + ((drawrect.y - pos.y) * pxm->index_xl);

	for (y = 0; y < drawrect.yl; y++) {
		for (x = 0; x < drawrect.xl; x++)
			if (pxm->index_data[read_offset++] < GFX_COLOR_INDEX_TRANSPARENT)
				map->index_data[offset++] = color;

		offset = base_offset += 320;
		read_offset = base_read_offset += pxm->index_xl;
	}
}

static int
_gfxop_install_pixmap(gfx_driver_t *driver, gfx_pixmap_t *pxm)
{
	int error;

	if (driver->capabilities & GFX_CAPABILITY_PIXMAP_REGISTRY
	    && !(pxm->flags & GFX_PIXMAP_FLAG_INSTALLED)) {
		error = driver->register_pixmap(driver, pxm);

		if (error) {
			GFXERROR("driver->register_pixmap() returned error!\n");
			return error;
		}
		pxm->flags |= GFX_PIXMAP_FLAG_INSTALLED;
	}

	if (driver->mode->palette &&
	    (!(pxm->flags & GFX_PIXMAP_FLAG_PALETTE_SET))) {
		int i;
		int error;

		for (i = 0; i < pxm->colors_nr; i++) {
			if ((error = driver->set_palette(driver, pxm->colors[i].global_index,
							 pxm->colors[i].r,
							 pxm->colors[i].g,
							 pxm->colors[i].b))) {

				GFXWARN("driver->set_palette(%d, %02x/%02x/%02x) failed!\n",
					pxm->colors[i].global_index,
					pxm->colors[i].r,
					pxm->colors[i].g,
					pxm->colors[i].b);

				if (error == GFX_FATAL)
					return GFX_FATAL;
			}
		}

		pxm->flags |= GFX_PIXMAP_FLAG_PALETTE_SET;
	}
	return GFX_OK;
}

static int
_gfxop_draw_pixmap(gfx_driver_t *driver, gfx_pixmap_t *pxm, int priority, int control,
		   rect_t src, rect_t dest, rect_t clip, int static_buf, gfx_pixmap_t *control_map)
{
	int error;
	rect_t clipped_dest = gfx_rect(dest.x, dest.y, dest.xl, dest.yl);
	if (control >= 0)
		_gfxop_draw_control(control_map, pxm, control,
				    gfx_point(dest.x / driver->mode->xfact,
					      dest.y / driver->mode->yfact));

	if (_gfxop_clip(&clipped_dest, clip))
		return GFX_OK;

	src.x += clipped_dest.x - dest.x;
	src.y += clipped_dest.y - dest.y;
	src.xl = clipped_dest.xl;
	src.yl = clipped_dest.yl;

	error = _gfxop_install_pixmap(driver, pxm);
	if (error) return error;

	error = driver->draw_pixmap(driver, pxm, priority, src, clipped_dest,
				    static_buf? GFX_BUFFER_STATIC : GFX_BUFFER_BACK);

	if (error) {
		GFXERROR("driver->draw_pixmap() returned error!\n");
		return error;
	}
	return GFX_OK;
}

static int
_gfxop_remove_pointer(gfx_state_t *state)
{
	if (state->mouse_pointer_visible
	    && !state->mouse_pointer_in_hw
	    && state->mouse_pointer_bg) {

		if (state->mouse_pointer_visible == POINTER_VISIBLE_BUT_CLIPPED) {
			state->mouse_pointer_visible = 0;
			return GFX_OK;
		}

		state->mouse_pointer_visible = 0;

		return
			state->driver->draw_pixmap(state->driver, state->mouse_pointer_bg, GFX_NO_PRIORITY,
						   gfx_rect(0, 0, state->mouse_pointer_bg->xl, state->mouse_pointer_bg->yl),
						   state->pointer_bg_zone,
						   GFX_BUFFER_BACK);

	} else return GFX_OK;
}

static int /* returns 1 if there are no pointer bounds, 0 otherwise */
_gfxop_get_pointer_bounds(gfx_state_t *state, rect_t *rect)
{
	gfx_pixmap_t *ppxm = state->mouse_pointer;

	if (!ppxm)
		return 0;

	rect->x = state->driver->pointer_x - ppxm->xoffset * (state->driver->mode->xfact);
	rect->y = state->driver->pointer_y - ppxm->yoffset * (state->driver->mode->yfact);
	rect->xl = ppxm->xl;
	rect->yl = ppxm->yl;

	return (_gfxop_clip(rect, gfx_rect(0, 0, 320 * state->driver->mode->xfact,
					   200 * state->driver->mode->yfact)));
}

static int
_gfxop_buffer_propagate_box(gfx_state_t *state, rect_t box, int buffer);

static int
_gfxop_draw_pointer(gfx_state_t *state)
{
	if (state->mouse_pointer_visible || !state->mouse_pointer || state->mouse_pointer_in_hw)
		return GFX_OK;
	else {
		int retval;
		gfx_pixmap_t *ppxm = state->mouse_pointer;
		int xfact, yfact;
		int x = state->driver->pointer_x - ppxm->xoffset * (xfact = state->driver->mode->xfact);
		int y = state->driver->pointer_y - ppxm->yoffset * (yfact = state->driver->mode->yfact);
		int error;

		state->mouse_pointer_visible = 1;

		state->old_pointer_draw_pos.x = x;
		state->old_pointer_draw_pos.y = y;

		retval = _gfxop_grab_pixmap(state, &(state->mouse_pointer_bg), x, y,
					    ppxm->xl, ppxm->yl, GFX_NO_PRIORITY,
					    &(state->pointer_bg_zone));

		if (retval == GFX_ERROR) {
			state->pointer_bg_zone = gfx_rect(320, 200, 0, 0);
			state->mouse_pointer_visible = POINTER_VISIBLE_BUT_CLIPPED;
			return GFX_OK;
		}

		if (retval)
			return retval;

		error = _gfxop_draw_pixmap(state->driver, ppxm, -1, -1,
					   gfx_rect(0, 0, ppxm->xl, ppxm->yl),
					   gfx_rect(x, y, ppxm->xl, ppxm->yl),
					   gfx_rect(0, 0, xfact * 320 , yfact * 200),
					   0, state->control_map);

		if (error)
			return error;


		return GFX_OK;
	}
}

gfx_pixmap_t *
_gfxr_get_cel(gfx_state_t *state, int nr, int *loop, int *cel)
{
	gfxr_view_t *view = gfxr_get_view(state->resstate, nr, loop, cel);
	gfxr_loop_t *indexed_loop;

	if (!view)
		return NULL;

	if (*loop >= view->loops_nr
	    || *loop < 0) {
		GFXWARN("Attempt to get cel from loop %d/%d inside view %d\n", *loop,
			view->loops_nr, nr);
		return NULL;
	}
	indexed_loop = view->loops + *loop;

	if (*cel >= indexed_loop->cels_nr
	    || *cel < 0) {
		GFXWARN("Attempt to get cel %d/%d from view %d/%d\n", *cel, indexed_loop->cels_nr,
			nr, *loop);
		return NULL;
	}

	return indexed_loop->cels[*cel]; /* Yes, view->cels uses a malloced pointer list. */
}

/*** Dirty rectangle operations ***/

static inline int
_gfxop_update_box(gfx_state_t *state, rect_t box)
{
	int retval;
	_gfxop_scale_rect(&box, state->driver->mode);

	if ((retval = _gfxop_buffer_propagate_box(state, box, GFX_BUFFER_FRONT))) {
		GFXERROR("Error occured while propagating box (%d,%d,%d,%d) to front buffer\n",
			 box.x, box.y, box.xl, box.yl);
		return retval;
	}
	return GFX_OK;
}


static struct _dirty_rect *
_rect_create(rect_t box)
{
	struct _dirty_rect *rect;

	rect = malloc(sizeof(struct _dirty_rect));
	rect->next = NULL;
	rect->rect = box;

	return rect;
}


gfx_dirty_rect_t *
gfxdr_add_dirty(gfx_dirty_rect_t *base, rect_t box, int strategy)
{
	if (box.xl < 0) {
		box.x += box.xl;
		box.xl = - box.xl;
	}

	if (box.yl < 0) {
		box.y += box.yl;
		box.yl = - box.yl;
	}

	if (_gfxop_clip(&box, gfx_rect(0, 0, 320, 200)))
		return base;

	switch (strategy) {

	case GFXOP_DIRTY_FRAMES_ONE:
		if (base)
			base->rect = gfx_rects_merge(box, base->rect);
		else 
			base = _rect_create(box);
		break;

	case GFXOP_DIRTY_FRAMES_CLUSTERS: {
		struct _dirty_rect **rectp = &(base);

		while (*rectp) {
			if (gfx_rects_overlap((*rectp)->rect, box)) {
				struct _dirty_rect *next = (*rectp)->next;
				box = gfx_rects_merge((*rectp)->rect, box);
				free(*rectp);
				*rectp = next;
			} else
				rectp = &((*rectp)->next);
		}
		*rectp = _rect_create(box);
		
	} break;

	default:
		GFXERROR("Attempt to use invalid dirty frame mode %d!\nPlease refer to gfx_options.h.", strategy);

	}

	return base;
}

static void
_gfxop_add_dirty(gfx_state_t *state, rect_t box)
{
	if (state->disable_dirty)
		return;

	state->dirty_rects = gfxdr_add_dirty(state->dirty_rects, box, state->options->dirty_frames);
}

static inline void
_gfxop_add_dirty_x(gfx_state_t *state, rect_t box)
     /* Extends the box size by one before adding (used for lines) */
{
	if (box.xl < 0)
		box.xl--;
	else
		box.xl++;

	if (box.yl < 0)
		box.yl--;
	else
		box.yl++;

	_gfxop_add_dirty(state, box);
}

static int
_gfxop_clear_dirty_rec(gfx_state_t *state, struct _dirty_rect *rect)
{
	int retval;

	if (!rect)
		return GFX_OK;

	retval = _gfxop_update_box(state, rect->rect);
	retval |= _gfxop_clear_dirty_rec(state, rect->next);

	free(rect);
	return retval;
}


/*** Exported operations ***/


static int
_gfxop_init_common(gfx_state_t *state, gfx_options_t *options)
{
	state->options = options;

	if ((state->static_palette = 
	     gfxr_interpreter_get_palette(state->version,
					  &(state->static_palette_entries))))
		_gfxop_alloc_colors(state, state->static_palette, state->static_palette_entries);

	if (!((state->resstate = gfxr_new_resource_manager(state->version, state->options, state->driver)))) {
		GFXERROR("Failed to initialize resource manager!\n");
		return GFX_FATAL;
	}

	state->visible_map = GFX_MASK_VISUAL;
	gfxop_set_clip_zone(state, gfx_rect(0, 0, 320, 200));

	if (!state->driver->capabilities & GFX_CAPABILITY_PIXMAP_GRABBING) {
		if (!state->driver->capabilities & GFX_CAPABILITY_MOUSE_POINTER) {
			GFXWARN("Graphics driver does not support drawing mouse pointers; disabling mouse input support.\n");
			state->driver->capabilities &= ~GFX_CAPABILITY_MOUSE_SUPPORT;
		} else if (gfxr_interpreter_needs_multicolored_pointers(state->version)
			   && !state->driver->capabilities & GFX_CAPABILITY_COLOR_MOUSE_POINTER) {
			GFXWARN("Graphics driver only supports monochrome mouse pointers, but colored pointers are needed; disabling mouse input support.\n");
			state->driver->capabilities &= ~GFX_CAPABILITY_MOUSE_SUPPORT;
		}

	}

	state->mouse_pointer = state->mouse_pointer_bg = NULL;
	state->mouse_pointer_visible = 0;
	state->control_map = gfx_pixmap_alloc_index_data(gfx_new_pixmap(320, 200, GFX_RESID_NONE, 0, 0));
	state->control_map->flags |= GFX_PIXMAP_FLAG_EXTERNAL_PALETTE;
	state->options = options;
	state->mouse_pointer_in_hw = 0;
	state->disable_dirty = 0;

	state->pic = state->pic_unscaled = NULL;

	state->pic_nr = -1; /* Set background pic number to an invalid value */

	state->tag_mode = 0;

	state->dirty_rects = NULL;

	return GFX_OK;
}

int
gfxop_init_default(gfx_state_t *state, gfx_options_t *options)
{
	BASIC_CHECKS(GFX_FATAL);
	if (state->driver->init(state->driver))
		return GFX_FATAL;

	return _gfxop_init_common(state, options);
}


int
gfxop_init(gfx_state_t *state, int xfact, int yfact, gfx_color_mode_t bpp, gfx_options_t *options)
{
	int color_depth = bpp? bpp : 1;
	int initialized = 0;
	BASIC_CHECKS(GFX_FATAL);

	do {
		if (!state->driver->init_specific(state->driver, xfact, yfact, color_depth))
			initialized = 1;
		else
			color_depth++;
	} while (!initialized && color_depth < 9 && !bpp);

	if (!initialized)
		return GFX_FATAL;

	return _gfxop_init_common(state, options);
}


int
gfxop_set_parameter(gfx_state_t *state, char *attribute, char *value)
{
	BASIC_CHECKS(GFX_FATAL);

	return state->driver->set_parameter(state->driver, attribute, value);
}


int
gfxop_exit(gfx_state_t *state)
{
	BASIC_CHECKS(GFX_ERROR);
	gfxr_free_resource_manager(state->driver, state->resstate);

	if (state->control_map) {
		gfx_free_pixmap(state->driver, state->control_map);
		state->control_map = NULL;
	}

	if (state->mouse_pointer_bg) {
		gfx_free_pixmap(state->driver, state->mouse_pointer_bg);
		state->mouse_pointer_bg = NULL;
	}

	state->driver->exit(state->driver);
	return GFX_OK;
}


static int
_gfxop_scan_one_bitmask(gfx_pixmap_t *pixmap, rect_t zone)
{
	int retval = 0;
	int startindex = (pixmap->index_xl * zone.y) + zone.x;

	if (_gfxop_clip(&zone, gfx_rect(0, 0, pixmap->index_xl, pixmap->index_yl)))
		return 0;

	while (zone.yl--) {
		int i;
		for (i = 0; i < zone.xl; i++)
			retval |= (1 << ((pixmap->index_data[startindex + i]) & 0xf));

		startindex += pixmap->index_xl;
	}

	return retval;
}

int
gfxop_scan_bitmask(gfx_state_t *state, rect_t area, gfx_map_mask_t map)
{
	gfxr_pic_t *pic = (state->pic_unscaled)? state->pic_unscaled : state->pic;
	int retval = 0;

	_gfxop_clip(&area, gfx_rect(0, 0, 320, 200));

	if (map & GFX_MASK_VISUAL)
		retval |= _gfxop_scan_one_bitmask(pic->visual_map, area);

	if (map & GFX_MASK_PRIORITY)
		retval |= _gfxop_scan_one_bitmask(pic->priority_map, area);

	if (map & GFX_MASK_CONTROL)
		retval |= _gfxop_scan_one_bitmask(state->control_map, area);

	return retval;
}


int
gfxop_set_visible_map(gfx_state_t *state, gfx_map_mask_t map)
{
	BASIC_CHECKS(GFX_ERROR);

	if (map != GFX_MASK_VISUAL
	    && map != GFX_MASK_PRIORITY
	    && map != GFX_MASK_CONTROL) {
		GFXWARN("Attempt to set invalid visible map #%d\n", map);
		return GFX_ERROR;
	}

	state->visible_map = map;
	return GFX_OK;
}


#define MIN_X 0
#define MIN_Y 0
#define MAX_X 319
#define MAX_Y 199

int
gfxop_set_clip_zone(gfx_state_t *state, rect_t zone)
{
	int xfact, yfact;
	BASIC_CHECKS(GFX_ERROR);

	xfact = state->driver->mode->xfact;
	yfact = state->driver->mode->yfact;

	if (zone.x < MIN_X) {
		zone.xl -= (zone.x - MIN_X);
		zone.x = MIN_X;
	}

	if (zone.y < MIN_Y) {
		zone.yl -= (zone.y - MIN_Y);
		zone.y = MIN_Y;
	}

	if (zone.x + zone.xl > MAX_X)
		zone.xl = MAX_X + 1 - zone.x;

	if (zone.y + zone.yl > MAX_Y)
		zone.yl = MAX_Y + 1 - zone.y;

	memcpy(&(state->clip_zone_unscaled), &zone, sizeof(rect_t));

	state->clip_zone.x = state->clip_zone_unscaled.x * xfact;
	state->clip_zone.y = state->clip_zone_unscaled.y * yfact;
	state->clip_zone.xl = state->clip_zone_unscaled.xl * xfact;
	state->clip_zone.yl = state->clip_zone_unscaled.yl * yfact;

	return GFX_OK;
}

int
gfxop_set_color(gfx_state_t *state, gfx_color_t *color, int r, int g, int b, int a,
		int priority, int control)
{
	gfx_pixmap_color_t pixmap_color;
	int error_code;
	int mask = ((r >= 0 && g >= 0 && b >= 0) ? GFX_MASK_VISUAL : 0)
		| ((priority >= 0)? GFX_MASK_PRIORITY : 0)
		| ((control >= 0)? GFX_MASK_CONTROL : 0);

	BASIC_CHECKS(GFX_FATAL);

	if (PALETTE_MODE && a >= GFXOP_ALPHA_THRESHOLD)
		mask &= ~GFX_MASK_VISUAL;

	color->mask = mask;

	if (mask & GFX_MASK_PRIORITY)
		color->priority = priority;

	if (mask & GFX_MASK_CONTROL)
		color->control = control;

	if (mask & GFX_MASK_VISUAL) {

		color->visual.r = r;
		color->visual.g = g;
		color->visual.b = b;
		color->alpha = a;

		if (PALETTE_MODE) {
			pixmap_color.r = r;
			pixmap_color.g = g;
			pixmap_color.b = b;
			pixmap_color.global_index = GFX_COLOR_INDEX_UNMAPPED;
			if ((error_code = gfx_alloc_color(state->driver->mode->palette, &pixmap_color))) {
				if (error_code < 0) {
					GFXWARN("Could not get color entry for %02x/%02x/%02x\n", r, g, b);
					return error_code;
				} else if ((error_code = state->driver->set_palette(state->driver, pixmap_color.global_index, r, g, b))) {
					GFXWARN("Graphics driver failed to set color index %d to (%02x/%02x/%02x)\n",
						pixmap_color.global_index, r, g, b);
					return error_code;
				}
			}
			color->visual.global_index = pixmap_color.global_index;
		}
	}
	return GFX_OK;
}

int
gfxop_set_system_color(gfx_state_t *state, gfx_color_t *color)
{
	gfx_palette_color_t *palette_colors;
	BASIC_CHECKS(GFX_FATAL);

	if (!PALETTE_MODE)
		return GFX_OK;

	if (color->visual.global_index < 0
	    || color->visual.global_index >= state->driver->mode->palette->max_colors_nr) {
		GFXERROR("Attempt to set invalid color index %02x as system color\n", color->visual.global_index);
		return GFX_ERROR;
	}

	palette_colors = state->driver->mode->palette->colors;
	palette_colors[color->visual.global_index].lockers = GFX_COLOR_SYSTEM;

	return GFX_OK;
}

int
gfxop_free_color(gfx_state_t *state, gfx_color_t *color)
{
	gfx_palette_color_t *palette_color;
	gfx_pixmap_color_t pixmap_color;
	int error_code;
	BASIC_CHECKS(GFX_FATAL);

	if (!PALETTE_MODE)
		return GFX_OK;

	if (color->visual.global_index < 0
	    || color->visual.global_index >= state->driver->mode->palette->max_colors_nr) {
		GFXERROR("Attempt to free invalid color index %02x\n", color->visual.global_index);
		return GFX_ERROR;
	}

	pixmap_color.global_index = color->visual.global_index;
	palette_color = state->driver->mode->palette->colors + pixmap_color.global_index;
	pixmap_color.r = palette_color->r;
	pixmap_color.g = palette_color->g;
	pixmap_color.b = palette_color->b;

	if ((error_code = gfx_free_color(state->driver->mode->palette, &pixmap_color))) {
		GFXWARN("Failed to free color with color index %02x\n", color->visual.global_index);
		return error_code;
	}

	return GFX_OK;
}

/******************************/
/* Generic drawing operations */
/******************************/


static int
line_check_bar(int *start, int *length, int clipstart, int cliplength)
{
	int overlength;

	if (*start < clipstart) {
		*length -= (clipstart - *start);
		*start = clipstart;
	}

	overlength = (*start + *length) - (clipstart + cliplength);

	if (overlength > 0)
		*length -= overlength;

	return (*length < 0);
}

static void
clip_line_partial(float *start, float *end, float delta_val, float pos_val, float start_val, float end_val)
{
	float my_start = (start_val - pos_val) * delta_val;
	float my_end = (end_val - pos_val) * delta_val;

	if (my_end < *end)
		*end = my_end;
	if (my_start > *start)
		*start = my_start;
}

static int
line_clip(rect_t *line, rect_t clip)
/* returns 1 if nothing is left, or 0 if part of the line is in the clip window */
{
	if (!line->xl) {/* vbar */
		if (line->x < clip.x || line->x >= (clip.x + clip.xl))
			return 1;

		return line_check_bar(&(line->y), &(line->yl), clip.y, clip.yl);

	} else

	if (!line->yl) {/* hbar */
		if (line->y < clip.y || line->y >= (clip.y + clip.yl))
			return 1;

		return line_check_bar(&(line->x), &(line->xl), clip.x, clip.xl);

	} else { /* "normal" line */
		float start = 0.0, end = 1.0;
		float xv = 1.0 * line->xl;
		float yv = 1.0 * line->yl;

		if (line->xl < 0)
			clip_line_partial(&start, &end, 1.0 / xv, 1.0 * line->x, 1.0 * (clip.x + clip.xl), 1.0 * clip.x);
		else
			clip_line_partial(&start, &end, 1.0 / xv, 1.0 * line->x, 1.0 * clip.x, 1.0 * (clip.x + clip.xl));

		if (line->yl < 0)
			clip_line_partial(&start, &end, 1.0 / yv, 1.0 * line->y, 1.0 * (clip.y + clip.yl), 1.0 * clip.y);
		else
			clip_line_partial(&start, &end, 1.0 / yv, 1.0 * line->y, 1.0 * clip.y, 1.0 * (clip.y + clip.yl));

		line->x += (int) xv * start;
		line->y += (int) yv * start;

		line->xl = (int) xv * (end-start);
		line->yl = (int) yv * (end-start);

		return (start > 1.0 || end < 0.0);
	}
	return 0;
}


static void
draw_line_to_control_map(gfx_state_t *state, rect_t line, gfx_color_t color)
{
	if (color.mask & GFX_MASK_CONTROL)
		if (!line_clip(&line, state->clip_zone_unscaled))
			gfx_draw_line_pixmap_i(state->control_map, line, color.control);
}

static int
simulate_stippled_line_draw(gfx_driver_t *driver, int skipone, rect_t line, gfx_color_t color, gfx_line_mode_t line_mode)
     /* Draws a stippled line if this isn't supported by the driver (skipone is ignored ATM) */
{
	int stepwidth = (line.xl)? driver->mode->xfact : driver->mode->yfact;
	int dbl_stepwidth = 2*stepwidth;
	int linelength = (line_mode == GFX_LINE_MODE_FINE)? stepwidth - 1 : 0;
	int *posvar = (line.xl)? &line.x : &line.y;
	int length = (line.xl)? line.xl : line.yl;
	int length_left = length;

	if (skipone) {
		length_left -= stepwidth;
		*posvar += stepwidth;
	}

	length /= dbl_stepwidth;

	length_left -= length * dbl_stepwidth;

	if (line.xl)
		line.xl = linelength;
	else
		line.yl = linelength;

	while (length--) {
		int retval;

		if ((retval = driver->draw_line(driver, line, color, line_mode, GFX_LINE_STYLE_NORMAL))) {
			GFXERROR("Failed to draw partial stippled line (%d,%d)+(%d,%d)\n", line.x, line.y, line.xl, line.yl);
			return retval;
		}
		*posvar += dbl_stepwidth;
	}

	if (length_left) {
		int retval;

		if (length_left > stepwidth)
			length_left = stepwidth;

		if (line.xl)
			line.xl = length_left;
		else
			if (line.yl) 
				line.yl = length_left;

		if ((retval = driver->draw_line(driver, line, color, line_mode, GFX_LINE_STYLE_NORMAL))) {
			GFXERROR("Failed to draw partial stippled line (%d,%d)+(%d,%d)\n", line.x, line.y, line.xl, line.yl);
			return retval;
		}
	}

	return GFX_OK;
}


static int
_gfxop_draw_line_clipped(gfx_state_t *state, rect_t line, gfx_color_t color, gfx_line_mode_t line_mode,
			 gfx_line_style_t line_style)
{
	int retval;
	int skipone = (line.x ^ line.y) & 1; /* Used for simulated line stippling */

	BASIC_CHECKS(GFX_FATAL);
	REMOVE_POINTER;

	/* First, make sure that the line is normalized */
	if (line.yl < 0) {
		line.y += line.yl;
		line.x += line.xl;
		line.yl = -line.yl;
		line.xl = -line.xl;
	}

	if (line.x < state->clip_zone.x
	    || (line.x + line.xl) < state->clip_zone.x
	    || line.y < state->clip_zone.y
	    || (line.x + line.xl) > (state->clip_zone.x + state->clip_zone.xl)
	    || line.x > (state->clip_zone.x + state->clip_zone.xl)
	    || (line.y + line.yl) > (state->clip_zone.y + state->clip_zone.yl))
		if (line_clip(&line, state->clip_zone))
			return GFX_OK; /* Clipped off */

	if (line_style == GFX_LINE_STYLE_STIPPLED) {
		if (line.xl && line.yl) {
			GFXWARN("Attempt to draw stippled line which is neither an hbar nor a vbar: (%d,%d)+(%d,%d)\n",
				line.x, line.y, line.xl, line.yl);
			return GFX_ERROR;
		}
		if (!(state->driver->capabilities & GFX_CAPABILITY_STIPPLED_LINES))
			return simulate_stippled_line_draw(state->driver, skipone, line, color, line_mode);
	}

	if ((retval = state->driver->draw_line(state->driver, line, color, line_mode, line_style))) {
		GFXERROR("Failed to draw line (%d,%d)+(%d,%d)\n", line.x, line.y, line.xl, line.yl);
		return retval;
	}
	return GFX_OK;
}
 
int
gfxop_draw_line(gfx_state_t *state, rect_t line, gfx_color_t color, gfx_line_mode_t line_mode,
		gfx_line_style_t line_style)
{
	int xfact, yfact;

	BASIC_CHECKS(GFX_FATAL);

	_gfxop_add_dirty_x(state, line);

	xfact = state->driver->mode->xfact;
	yfact = state->driver->mode->yfact;

	draw_line_to_control_map(state, line, color);

	_gfxop_scale_rect(&line, state->driver->mode);

	line.x += xfact >> 1;
	line.y += yfact >> 1;

	return _gfxop_draw_line_clipped(state, line, color, line_mode, line_style);
}
 
int
gfxop_draw_rectangle(gfx_state_t *state, rect_t rect, gfx_color_t color, gfx_line_mode_t line_mode,
		     gfx_line_style_t line_style)
{
	int retval = 0;
	rect_t line, unscaled_line = gfx_rect(rect.x, rect.y, rect.xl, rect.yl);
	int xfact, yfact;
	int xunit, yunit;
	int ystart;
	int xl, yl;

	BASIC_CHECKS(GFX_FATAL);
	REMOVE_POINTER;

	xfact = state->driver->mode->xfact;
	yfact = state->driver->mode->yfact;

	if (rect.xl == 0 || rect.yl == 0) {
		GFXWARN("Rectangle too small: (%d,%d)+(%d,%d)\n", rect.x, rect.y, rect.xl, rect.yl);
		return GFX_ERROR;
	}

	if (line_mode == GFX_LINE_MODE_FINE) {
		xunit = yunit = 1;
		xl = 1 + (rect.xl - 1) * xfact;
		yl = 1 + (rect.yl - 1) * yfact;
		line.x = rect.x * xfact + (xfact - 1);
		line.y = rect.y * yfact + (yfact - 1);
	} else {
		xunit = xfact;
		yunit = yfact;
		xl = rect.xl * xfact;
		yl = rect.yl * yfact;
		line.x = rect.x * xfact + (xfact >> 1);
		line.y = rect.y * yfact + (yfact >> 1);
	}

	ystart = line.y;
	line.xl = xl;
	line.yl = 0;
	retval |= _gfxop_draw_line_clipped(state, line, color, line_mode, line_style);
	unscaled_line.yl = 0;
	draw_line_to_control_map(state, unscaled_line, color);
	_gfxop_add_dirty_x(state, unscaled_line);


	line.y += yl;
	line.x += xunit;
	line.xl -= xunit;
	retval |= _gfxop_draw_line_clipped(state, line, color, line_mode, line_style);
	unscaled_line.y += rect.yl;
	draw_line_to_control_map(state, unscaled_line, color);
	_gfxop_add_dirty_x(state, unscaled_line);

	line.xl = 0;
	line.yl = yl - yunit;
	line.y = ystart + yunit;
	line.x -= xunit;
	retval |= _gfxop_draw_line_clipped(state, line, color, line_mode, line_style);
	unscaled_line.y = rect.y;
	unscaled_line.yl = rect.yl;
	unscaled_line.xl = rect.xl;
	draw_line_to_control_map(state, unscaled_line, color);
	_gfxop_add_dirty_x(state, unscaled_line);

	line.x += xl;
	line.y = ystart;
	retval |= _gfxop_draw_line_clipped(state, line, color, line_mode, line_style);
	unscaled_line.x += rect.xl;
	draw_line_to_control_map(state, unscaled_line, color);
	_gfxop_add_dirty_x(state, unscaled_line);

	if (retval) {
		GFXERROR("Failed to draw rectangle (%d,%d)+(%d,%d)\n", rect.x, rect.y, rect.xl, rect.yl);
		return retval;
	}
	return GFX_OK;
}


#define COLOR_MIX(type, dist) ((color1.type * dist) + (color2.type * (1.0 - dist)))


int
gfxop_draw_box(gfx_state_t *state, rect_t box, gfx_color_t color1, gfx_color_t color2,
	       gfx_box_shade_t shade_type)
{
	gfx_driver_t *drv = state->driver;
	int reverse = 0; /* switch color1 and color2 */
	float mod_offset = 0.0, mod_breadth = 1.0; /* 0.0 to 1.0: Color adjustment */
	int driver_shade_type;
	rect_t new_box;
	gfx_color_t draw_color1, draw_color2;

	BASIC_CHECKS(GFX_FATAL);
	REMOVE_POINTER;

	if (PALETTE_MODE || !(state->driver->capabilities & GFX_CAPABILITY_SHADING))
		shade_type = GFX_BOX_SHADE_FLAT;


	_gfxop_add_dirty(state, box);

	if (color1.mask & GFX_MASK_CONTROL) {
		/* Write control block, clipped by 320x200 */
		memcpy(&new_box, &box, sizeof(rect_t));
		_gfxop_clip(&new_box, gfx_rect(0, 0, 320, 200));

		gfx_draw_box_pixmap_i(state->control_map, new_box, color1.control);
	}

	_gfxop_scale_rect(&box, state->driver->mode);

	if (!(color1.mask & (GFX_MASK_VISUAL | GFX_MASK_PRIORITY)))
		return GFX_OK; /* So long... */

	if (box.xl <= 1 || box.yl <= 1) {
		GFXDEBUG("Attempt to draw box with size %dx%d\n", box.xl, box.yl);
		return GFX_OK;
	}

	memcpy(&new_box, &box, sizeof(rect_t));

	if (_gfxop_clip(&new_box, state->clip_zone))
		return GFX_OK;

	switch (shade_type) {

	case GFX_BOX_SHADE_FLAT:
		driver_shade_type = GFX_SHADE_FLAT;
		break;

	case GFX_BOX_SHADE_LEFT: reverse = 1;
	case GFX_BOX_SHADE_RIGHT:
		driver_shade_type = GFX_SHADE_HORIZONTALLY;
		mod_offset = ((new_box.x - box.x) * 1.0) / (box.xl * 1.0);
		mod_breadth = (new_box.xl * 1.0) / (box.xl * 1.0);
		break;

	case GFX_BOX_SHADE_UP: reverse = 1;
	case GFX_BOX_SHADE_DOWN:
		driver_shade_type = GFX_SHADE_VERTICALLY;
		mod_offset = ((new_box.y - box.y) * 1.0) / (box.yl * 1.0);
		mod_breadth = (new_box.yl * 1.0) / (box.yl * 1.0);
		break;

	default:
		GFXERROR("Invalid shade type: %d\n", shade_type);
		return GFX_ERROR;
	}


	if (reverse)
		mod_offset = 1.0 - (mod_offset + mod_breadth);
	/* Reverse offset if we have to interpret colors inversely */

	if (shade_type == GFX_BOX_SHADE_FLAT)
		return drv->draw_filled_rect(drv, new_box, color1, color1, GFX_SHADE_FLAT);
	else {
		if (PALETTE_MODE) {
			GFXWARN("Attempting to draw shaded box in palette mode!\n");
			return GFX_ERROR;
		}

		draw_color1.mask = draw_color2.mask = color1.mask;
		draw_color1.priority = draw_color2.priority = color1.priority;

		if (draw_color1.mask & GFX_MASK_VISUAL) {
			draw_color1.visual.r = COLOR_MIX(visual.r, mod_offset);
			draw_color1.visual.g = COLOR_MIX(visual.g, mod_offset);
			draw_color1.visual.b = COLOR_MIX(visual.b, mod_offset);
			draw_color1.alpha = COLOR_MIX(alpha, mod_offset);

			mod_offset += mod_breadth;

			draw_color2.visual.r = COLOR_MIX(visual.r, mod_offset);
			draw_color2.visual.g = COLOR_MIX(visual.g, mod_offset);
			draw_color2.visual.b = COLOR_MIX(visual.b, mod_offset);
			draw_color2.alpha = COLOR_MIX(alpha, mod_offset);
		}
		if (reverse)
			return drv->draw_filled_rect(drv, new_box, draw_color2, draw_color1, driver_shade_type);
		else
			return drv->draw_filled_rect(drv, new_box, draw_color1, draw_color2, driver_shade_type);
	}
}
#undef COLOR_MIX


int
gfxop_fill_box(gfx_state_t *state, rect_t box, gfx_color_t color)
{
	return gfxop_draw_box(state, box, color, color, GFX_BOX_SHADE_FLAT);
}



static int
_gfxop_buffer_propagate_box(gfx_state_t *state, rect_t box, int buffer)
{
	int error;

	if (_gfxop_clip(&box, gfx_rect(0, 0, 320 * state->driver->mode->xfact, 200 * state->driver->mode->yfact)))
		return GFX_OK;

	if ((error = state->driver->update(state->driver, box, gfx_point(box.x, box.y), buffer))) {
		GFXERROR("Error occured while updating region (%d,%d,%d,%d) in buffer %d\n",
			 box.x, box.y, box.xl, box.yl, buffer);
		return error;
	}
	return GFX_OK;
}


int
gfxop_clear_box(gfx_state_t *state, rect_t box)
{
	BASIC_CHECKS(GFX_FATAL);
	REMOVE_POINTER;
	_gfxop_add_dirty(state, box);

	_gfxop_scale_rect(&box, state->driver->mode);

	return _gfxop_buffer_propagate_box(state, box, GFX_BUFFER_BACK);
}


int
gfxop_update(gfx_state_t *state)
{
	int retval;

	BASIC_CHECKS(GFX_FATAL);
	DRAW_POINTER;

	retval = _gfxop_clear_dirty_rec(state, state->dirty_rects);

	state->dirty_rects = NULL;

	if (retval) {
		GFXERROR("Clearing the dirty rectangles failed!\n");
	}

	if (state->tag_mode) {
		/* This usually happens after a pic and all resources have been drawn */
		gfxr_free_tagged_resources(state->driver, state->resstate);
		state->tag_mode = 0;
	}

	return retval;
}


int
gfxop_update_box(gfx_state_t *state, rect_t box)
{
	BASIC_CHECKS(GFX_FATAL);
	DRAW_POINTER;

	if (state->disable_dirty)
		_gfxop_update_box(state, box);
	else
		_gfxop_add_dirty(state, box);

	return gfxop_update(state);
}

int
gfxop_enable_dirty_frames(gfx_state_t *state)
{
	BASIC_CHECKS(GFX_ERROR);
	state->disable_dirty = 0;

	return GFX_OK;
}

int
gfxop_disable_dirty_frames(gfx_state_t *state)
{
	BASIC_CHECKS(GFX_ERROR);
	state->disable_dirty = 1;

	return GFX_OK;
}


/**********************/
/* Pointer and IO ops */
/**********************/

#define SECONDS_OF_DAY (24*60*60)
#define MILLION 1000000
/* Sure, this may seem silly, but it's too easy to miss a zero...) */


#define GFXOP_FULL_POINTER_REFRESH if (_gfxop_full_pointer_refresh(state)) { GFXERROR("Failed to do full pointer refresh!\n"); return GFX_ERROR; }

static int
_gfxop_full_pointer_refresh(gfx_state_t *state)
{
	rect_t pointer_bounds, old_pointer_bounds;
	int new_x = state->driver->pointer_x;
	int new_y = state->driver->pointer_y;

	if (new_x != state->old_pointer_draw_pos.x
	    || new_y != state->old_pointer_draw_pos.y) {	

		if (!_gfxop_get_pointer_bounds(state, &pointer_bounds)) {
			memcpy(&old_pointer_bounds, &(state->pointer_bg_zone), sizeof(rect_t));
			REMOVE_POINTER;
			state->pointer_pos.x = state->driver->pointer_x / state->driver->mode->xfact;
			state->pointer_pos.y = state->driver->pointer_y / state->driver->mode->yfact;
			DRAW_POINTER;
			if (_gfxop_buffer_propagate_box(state, pointer_bounds, GFX_BUFFER_FRONT)) return 1;
			if (_gfxop_buffer_propagate_box(state, old_pointer_bounds, GFX_BUFFER_FRONT)) return 1;

			state->old_pointer_draw_pos = gfx_point(new_x, new_y);
		}
	}
	return 0;
}

int
gfxop_usleep(gfx_state_t *state, int usecs)
{
	int time, utime;
	int wakeup_time, wakeup_utime;
	int add_seconds;
	int retval = GFX_OK;

	BASIC_CHECKS(GFX_FATAL);

	sci_gettime(&wakeup_time, &wakeup_utime);
	wakeup_utime += usecs;

	add_seconds = (wakeup_utime / MILLION);
	wakeup_time += add_seconds;
	wakeup_utime -= (MILLION * add_seconds);

	do {
		GFXOP_FULL_POINTER_REFRESH;

		sci_gettime(&time, &utime);
		usecs = (wakeup_time - time) * MILLION + wakeup_utime - utime;
	} while ((usecs > 0) && !(retval = state->driver->usec_sleep(state->driver, usecs)));

	if (retval) {
		GFXWARN("Waiting failed\n");
	}

	return retval;
}


int
_gfxop_set_pointer(gfx_state_t *state, gfx_pixmap_t *pxm)
{
	rect_t old_pointer_bounds, pointer_bounds;
	int retval;
	int draw_old;
	int draw_new = 0;

	BASIC_CHECKS(GFX_FATAL);

	draw_old = state->mouse_pointer != NULL;

	if (state->driver->capabilities & GFX_CAPABILITY_MOUSE_POINTER) {

		if (draw_old && state->mouse_pointer->colors_nr > 2)
			draw_old = state->driver->capabilities & GFX_CAPABILITY_COLOR_MOUSE_POINTER;

		if (!draw_old
		    && state->mouse_pointer
		    && (state->driver->capabilities & GFX_CAPABILITY_POINTER_PIXMAP_REGISTRY))
			if ((retval = state->driver->unregister_pixmap(state->driver, state->mouse_pointer))){
				GFXERROR("Pointer un-registration failed!\n");
				return retval;
			}

		if (pxm == NULL
		    || (state->driver->capabilities & GFX_CAPABILITY_COLOR_MOUSE_POINTER)
		    || pxm->colors_nr <= 2) {
			if (state->driver->capabilities & GFX_CAPABILITY_POINTER_PIXMAP_REGISTRY) {
				if ((pxm) && (retval = state->driver->register_pixmap(state->driver, pxm))) {
					GFXERROR("Pixmap-registering a new mouse pointer failed!\n");
					return retval;
				}
			}
			draw_new = 0;
			state->driver->set_pointer(state->driver, pxm);
			state->mouse_pointer_in_hw = 1;
		} else {
			draw_new = 1;
			state->mouse_pointer_in_hw = 0;
		}

	} else draw_new = 1;

	if (draw_old) {
		_gfxop_get_pointer_bounds(state, &old_pointer_bounds);
		REMOVE_POINTER;
	}


	if (draw_new) {
		state->mouse_pointer = pxm;
		DRAW_POINTER;
		_gfxop_get_pointer_bounds(state, &pointer_bounds);
	}

	if (draw_new && state->mouse_pointer)
		_gfxop_buffer_propagate_box(state, pointer_bounds, GFX_BUFFER_FRONT);

	if (draw_old)
		_gfxop_buffer_propagate_box(state, old_pointer_bounds, GFX_BUFFER_FRONT);

	if (state->mouse_pointer == NULL)
		state->mouse_pointer_visible = 0;
	else if (!state->mouse_pointer_visible)
		state->mouse_pointer_visible = 1;
	/* else don't touch it, as it might be VISIBLE_BUT_CLIPPED! */

	return GFX_OK;
}


int
gfxop_set_pointer_cursor(gfx_state_t *state, int nr)
{
	gfx_pixmap_t *new_pointer;

	BASIC_CHECKS(GFX_FATAL);

	if (nr == GFXOP_NO_POINTER)
		new_pointer = NULL;
	else {
		new_pointer = gfxr_get_cursor(state->resstate, nr);

		if (!new_pointer) {
			GFXWARN("Attempt to set invalid pointer #%d\n", nr);
		}
	}

	return _gfxop_set_pointer(state, new_pointer);
}


int
gfxop_set_pointer_view(gfx_state_t *state, int nr, int loop, int cel)
{
	int real_loop = loop;
	int real_cel = cel;
	gfx_pixmap_t *new_pointer;

	BASIC_CHECKS(GFX_FATAL);

	new_pointer = _gfxr_get_cel(state, nr, &real_loop, &real_cel);


	if (!state->mouse_pointer) {
		GFXWARN("Attempt to set invalid pointer #%d\n", nr);
	} else if (real_loop != loop || real_cel != cel) {
		GFXDEBUG("Changed loop/cel from %d/%d to %d/%d in view %d\n",
			 loop, cel, real_loop, real_cel, nr);
	}

	return _gfxop_set_pointer(state, new_pointer);
}

int
gfxop_set_pointer_position(gfx_state_t *state, point_t pos)
{
	BASIC_CHECKS(GFX_ERROR);

	state->pointer_pos = pos;
	state->driver->pointer_x = pos.x * state->driver->mode->xfact;
	state->driver->pointer_y = pos.y * state->driver->mode->yfact;

	GFXOP_FULL_POINTER_REFRESH;
	return 0;
}

sci_event_t
gfxop_get_event(gfx_state_t *state)
{
	sci_event_t error_event = { SCI_EVT_ERROR, 0, 0 };
	sci_event_t event;

	BASIC_CHECKS(error_event);

	if (_gfxop_remove_pointer(state)) {
		GFXERROR("Failed to remove pointer before processing event!\n");
	}

	event = state->driver->get_event(state->driver);

	if (_gfxop_full_pointer_refresh(state)) {
		GFXERROR("Failed to update the mouse pointer!\n");
		return error_event;
	}

	return event;
}


/*******************/
/* View operations */
/*******************/

int
gfxop_lookup_view_get_loops(gfx_state_t *state, int nr)
{
	int loop = 0, cel = 0;
	gfxr_view_t *view;

	BASIC_CHECKS(GFX_ERROR);

	view = gfxr_get_view(state->resstate, nr, &loop, &cel);

	if (!view) {
		GFXWARN("Attempt to retreive number of loops from invalid view %d\n", nr);
		return 0;
	}

	return view->loops_nr;
}


int
gfxop_lookup_view_get_cels(gfx_state_t *state, int nr, int loop)
{
	int real_loop = loop, cel = 0;
	gfxr_view_t *view;

	BASIC_CHECKS(GFX_ERROR);

	view = gfxr_get_view(state->resstate, nr, &real_loop, &cel);

	if (!view) {
		GFXWARN("Attempt to retreive number of cels from invalid view %d\n", nr);
		return 0;
	} else if (real_loop != loop) {
		GFXWARN("Loop number was corrected from %d to %d in view %d\n", loop, real_loop, nr);
	}

	return view->loops[real_loop].cels_nr;
}


int
gfxop_check_cel(gfx_state_t *state, int nr, int *loop, int *cel)
{
        BASIC_CHECKS(GFX_ERROR);

	if (!gfxr_get_view(state->resstate, nr, loop, cel)) {
		GFXWARN("Attempt to verify loop/cel values for invalid view %d\n", nr);
		return GFX_ERROR;
	}

	return GFX_OK;
}

int
gfxop_overflow_cel(gfx_state_t *state, int nr, int *loop, int *cel)
{
	int loop_v = *loop;
	int cel_v = *cel;
        BASIC_CHECKS(GFX_ERROR);

	if (!gfxr_get_view(state->resstate, nr, &loop_v, &cel_v)) {
		GFXWARN("Attempt to verify loop/cel values for invalid view %d\n", nr);
		return GFX_ERROR;
	}

	if (loop_v != *loop)
		*loop = 0;

	if (loop_v != *loop
	    || cel_v != *cel)
		*cel = 0;

	return GFX_OK;
}


int
gfxop_get_cel_parameters(gfx_state_t *state, int nr, int loop, int cel,
			 int *width, int *height, point_t *offset)
{
	gfxr_view_t *view;
	gfx_pixmap_t *pxm;
	BASIC_CHECKS(GFX_ERROR);

	if (!(view = gfxr_get_view(state->resstate, nr, &loop, &cel))) {
		GFXWARN("Attempt to get cel parameters for invalid view %d\n", nr);
		return GFX_ERROR;
	}

	pxm = view->loops[loop].cels[cel];
	*width = pxm->index_xl;
	*height = pxm->index_yl;
	offset->x = pxm->xoffset;
	offset->y = pxm->yoffset;

	return GFX_OK;
}


static int
_gfxop_draw_cel_buffer(gfx_state_t *state, int nr, int loop, int cel,
		       point_t pos, gfx_color_t color, int static_buf)
{
	int priority = (color.mask & GFX_MASK_PRIORITY)? color.priority : -1;
	int control = (color.mask & GFX_MASK_CONTROL)? color.control : -1;
	gfxr_view_t *view;
	gfx_pixmap_t *pxm;
	int old_x, old_y;
	BASIC_CHECKS(GFX_FATAL);

	if (!(view = gfxr_get_view(state->resstate, nr, &loop, &cel))) {
		GFXWARN("Attempt to draw loop/cel %d/%d in invalid view %d\n", loop, cel, nr);
		return GFX_ERROR;
	}

	pxm = view->loops[loop].cels[cel];

	old_x = pos.x -= pxm->xoffset;
	old_y = pos.y -= pxm->yoffset;

	pos.x *= state->driver->mode->xfact;
	pos.y *= state->driver->mode->yfact;

	if (!static_buf)
		_gfxop_add_dirty(state, gfx_rect(old_x, old_y, pxm->index_xl, pxm->index_yl));

	return _gfxop_draw_pixmap(state->driver, pxm, priority, control,
				  gfx_rect(0, 0, pxm->xl, pxm->yl),
				  gfx_rect(pos.x, pos.y, pxm->xl, pxm->yl),
				  state->clip_zone,
				  static_buf , state->control_map);
}


int
gfxop_draw_cel(gfx_state_t *state, int nr, int loop, int cel, point_t pos,
	       gfx_color_t color)
{
	return _gfxop_draw_cel_buffer(state, nr, loop, cel, pos, color, 0);
}


int
gfxop_draw_cel_static(gfx_state_t *state, int nr, int loop, int cel, point_t pos,
		      gfx_color_t color)
{
	return _gfxop_draw_cel_buffer(state, nr, loop, cel, pos, color, 1);
}


/******************/
/* Pic operations */
/******************/

static int
_gfxop_set_pic(gfx_state_t *state)
{
	gfx_pixmap_t *pxm = NULL;
	byte unscaled = (state->options->pic0_unscaled);
	gfx_copy_pixmap_box_i(state->control_map, state->pic->control_map, gfx_rect(0, 0, 320, 200));
	state->control_map->colors_nr = state->pic->control_map->colors_nr;
	state->control_map->colors = state->pic->control_map->colors;


	switch (state->visible_map) {

	case GFX_MASK_VISUAL:
		pxm = unscaled? state->pic_unscaled->visual_map : state->pic->visual_map;
		break;

	case GFX_MASK_PRIORITY:
		pxm = state->pic->priority_map;
		break;

	case GFX_MASK_CONTROL:
		pxm = state->pic->control_map;
		break;


	default:
		GFXERROR("Attempt to draw invalid map #%d!\n", state->visible_map);
		return GFX_ERROR;
	}

	_gfxop_install_pixmap(state->driver, pxm);
	return state->driver->set_static_buffer(state->driver, pxm, state->pic->priority_map);
}

int
gfxop_new_pic(gfx_state_t *state, int nr, int flags, int default_palette)
{
	BASIC_CHECKS(GFX_FATAL);

	gfxr_tag_resources(state->resstate);
	state->tag_mode = 1;

	state->pic = gfxr_get_pic(state->resstate, nr, state->visible_map, flags, default_palette, 1);

	if (state->driver->mode->xfact == 1 && state->driver->mode->yfact == 1)
		state->pic_unscaled = state->pic;
	else
		state->pic_unscaled = gfxr_get_pic(state->resstate, nr, state->visible_map, flags, default_palette, 0);

	if (!state->pic || !state->pic_unscaled) {
		GFXERROR("Could not retreive background pic %d!\n", nr);
		if (state->pic) {
			GFXERROR("  -- Inconsistency: scaled pic _was_ retreived!\n");
		}

		if (state->pic_unscaled) {
			GFXERROR("  -- Inconsistency: unscaled pic _was_ retreived!\n");
		}

		state->pic = state->pic_unscaled = NULL;
		return GFX_ERROR;
	}

	state->pic_nr = nr;
	GFXDEBUG("Setting new pic %d\n", nr);
	return _gfxop_set_pic(state);
}


int
gfxop_add_to_pic(gfx_state_t *state, int nr, int flags, int default_palette)
{
	BASIC_CHECKS(GFX_FATAL);

	if (!state->pic) {
		GFXERROR("Attempt to add to pic with no pic active!\n");
		return GFX_ERROR;
	}

	if (!gfxr_add_to_pic(state->resstate, state->pic_nr, nr,
			     state->visible_map, flags, default_palette, 1)) {
		GFXERROR("Could not add pic #%d to pic #%d!\n", state->pic_nr, nr);
		return GFX_ERROR;
	}
	return _gfxop_set_pic(state);
}


/*******************/
/* Text operations */
/*******************/


int
gfxop_get_font_height(gfx_state_t *state, int font_nr)
{
	gfx_bitmap_font_t *font;
	BASIC_CHECKS(GFX_FATAL);

	font = gfxr_get_font(state->resstate, font_nr, 0);
	if (!font)
		return GFX_ERROR;

	return font->line_height;
}

int
gfxop_get_text_params(gfx_state_t *state, int font_nr, char *text,
		      int maxwidth, int *width, int *height)
{
	text_fragment_t *textsplits;
	gfx_bitmap_font_t *font;
	int lines;

	BASIC_CHECKS(GFX_FATAL);

	font = gfxr_get_font(state->resstate, font_nr, 0);

	if (!font) {
		GFXERROR("Attempt to calculate text size with invalid font #%d\n", font_nr);
		*width = *height = 0;
		return GFX_ERROR;
	}

	textsplits = gfxr_font_calculate_size(font, maxwidth, text, width, height, &lines,
					      state->options->workarounds & GFX_WORKAROUND_WHITESPACE_COUNT);


	if (!textsplits) {
		GFXERROR("Could not calculate text size!");
		*width = *height = 0;
		return GFX_ERROR;
	}

	free(textsplits);
	return GFX_OK;
}


#define COL_XLATE(des,src) \
  des = src.visual; /* The new gfx_color_t structure makes things a lot easier :-) */


gfx_text_handle_t *
gfxop_new_text(gfx_state_t *state, int font_nr, char *text, int maxwidth,
	       gfx_alignment_t halign, gfx_alignment_t valign,
	       gfx_color_t color1, gfx_color_t color2, gfx_color_t bg_color,
	       int flags)
{
	gfx_text_handle_t *handle;
	gfx_bitmap_font_t *font;
	int i;
	gfx_pixmap_color_t pxm_col1, pxm_col2, pxm_colbg;
	BASIC_CHECKS(NULL);

	COL_XLATE(pxm_col1, color1);
	COL_XLATE(pxm_col2, color2);
	COL_XLATE(pxm_colbg, bg_color);

	font = gfxr_get_font(state->resstate, font_nr, 0);

	if (!font) {
		GFXERROR("Attempt to draw text with invalid font #%d\n", font_nr);
		return NULL;
	}

	handle = malloc(sizeof(gfx_text_handle_t));

	handle->text = malloc(strlen(text) + 1);
	strcpy(handle->text, text);
	handle->halign = halign;
	handle->valign = valign;
	handle->line_height = font->line_height;

	handle->lines =
		gfxr_font_calculate_size(font, maxwidth, handle->text, &(handle->width), &(handle->height),
					 &(handle->lines_nr),
					 ((state->options->workarounds & GFX_WORKAROUND_WHITESPACE_COUNT)?
					  GFXR_FONT_FLAG_COUNT_WHITESPACE : 0)
					 | flags);

	if (!handle->lines) {
		free(handle->text);
		free(handle);
		GFXERROR("Could not calculate text parameters in font #%d\n", font_nr);
		return NULL;
	}


	if (flags & GFXR_FONT_FLAG_NO_NEWLINES) {
		handle->lines_nr = 1;
		handle->lines->length = strlen(text);
	}

	handle->text_pixmaps = malloc(sizeof(gfx_pixmap_t *) * handle->lines_nr);

	for (i = 0; i < handle->lines_nr; i++) {
		int chars_nr = handle->lines[i].length;

		handle->text_pixmaps[i] = gfxr_draw_font(font, handle->lines[i].offset, chars_nr,
							 (color1.mask & GFX_MASK_VISUAL)? &pxm_col1 : NULL,
							 (color2.mask & GFX_MASK_VISUAL)? &pxm_col2 : NULL,
							 (bg_color.mask & GFX_MASK_VISUAL)? &pxm_colbg : NULL);

		if (!handle->text_pixmaps[i]) {
			int j;

			for (j = 0; j < i; j++)
				gfx_free_pixmap(state->driver, handle->text_pixmaps[j]);
			free(handle->text_pixmaps);
			free(handle->text);
			free(handle->lines);
			free(handle);
			GFXERROR("Failed to draw text pixmap for line %d/%d\n", i, handle->lines_nr);
			return NULL;
		}
	}

	handle->font = font;

	handle->priority = (color1.mask & GFX_MASK_PRIORITY)? color1.priority : -1;
	handle->control = (color1.mask & GFX_MASK_CONTROL)? color1.control : -1;

	return handle;
}


int
gfxop_free_text(gfx_state_t *state, gfx_text_handle_t *handle)
{
	int j;

	BASIC_CHECKS(GFX_ERROR);

	if (handle->text_pixmaps) {
		for (j = 0; j < handle->lines_nr; j++)
			gfx_free_pixmap(state->driver, handle->text_pixmaps[j]);
		free(handle->text_pixmaps);
	}

	free(handle->text);
	free(handle->lines);
	free(handle);
	return GFX_OK;
}


int
gfxop_draw_text(gfx_state_t *state, gfx_text_handle_t *handle, rect_t zone)
{
	int line_height;
	rect_t pos;
	int i;
	BASIC_CHECKS(GFX_FATAL);
	REMOVE_POINTER;

	if (!handle) {
		GFXERROR("Attempt to draw text with NULL handle!\n");
		return GFX_ERROR;
	}

	if (!handle->lines_nr) {
		GFXDEBUG("Skipping draw_text operation because number of lines is zero\n");
		return GFX_OK;
	}

	_gfxop_scale_rect(&zone, state->driver->mode);

	line_height = handle->line_height * state->driver->mode->yfact;

	pos.y = zone.y;

	switch (handle->valign) {

	case ALIGN_TOP:
		break;

	case ALIGN_CENTER:
		pos.y += (zone.yl - (line_height * handle->lines_nr)) >> 1;
		break;

	case ALIGN_BOTTOM:
		pos.y += (zone.yl - (line_height * handle->lines_nr));
		break;

	default: 
		GFXERROR("Invalid vertical alignment %d!\n", handle->valign);
		return GFX_FATAL; /* Internal error... */
	}

	for (i = 0; i < handle->lines_nr; i++) {

		gfx_pixmap_t *pxm = handle->text_pixmaps[i];

		if (!pxm->data)
			gfx_xlate_pixmap(pxm, state->driver->mode, state->options->text_xlate_filter);

		if (!pxm) {
			GFXERROR("Could not find text pixmap %d/%d\n", i, handle->lines_nr);
			return GFX_ERROR;
		}

		pos.x = zone.x;

		switch (handle->halign) {

		case ALIGN_LEFT:
			break;

		case ALIGN_CENTER:
			pos.x += (zone.xl - pxm->xl) >> 1;
			break;

		case ALIGN_RIGHT:
			pos.x += (zone.xl - pxm->xl);
			break;

		default: 
			GFXERROR("Invalid vertical alignment %d!\n", handle->valign);
			return GFX_FATAL; /* Internal error... */
		}

		pos.xl = pxm->xl;
		pos.yl = pxm->yl;

		_gfxop_add_dirty(state, pos);

		_gfxop_draw_pixmap(state->driver, pxm, handle->priority, handle->control,
				   gfx_rect(0, 0, pxm->xl, pxm->yl), pos, state->clip_zone, 0,
				   state->control_map);

		pos.y += line_height;
	}

	return GFX_OK;
}


gfx_pixmap_t *
gfxop_grab_pixmap(gfx_state_t *state, rect_t area)
{
	gfx_pixmap_t *pixmap = NULL;
	rect_t resultzone; /* Ignored for this application */
	BASIC_CHECKS(NULL);

	if (_gfxop_remove_pointer(state)) {
		GFXERROR("Could not remove pointer!\n");
		return NULL;
	}

	_gfxop_scale_rect(&area, state->driver->mode);
	if (_gfxop_grab_pixmap(state, &pixmap, area.x, area.y, area.xl, area.yl, 0, &resultzone))
		return NULL; /* area CUT the visual screen had a null or negative size */

	pixmap->flags |= GFX_PIXMAP_FLAG_PALETTE_SET | GFX_PIXMAP_FLAG_DONT_UNALLOCATE_PALETTE;

	return pixmap;
}

int
gfxop_draw_pixmap(gfx_state_t *state, gfx_pixmap_t *pxm, rect_t zone, point_t pos)
{
	rect_t target;
	BASIC_CHECKS(GFX_ERROR);

	if (!pxm) {
		GFXERROR("Attempt to draw NULL pixmap!\n");
		return GFX_ERROR;
	}

	REMOVE_POINTER;

	target = gfx_rect(pos.x, pos.y, zone.xl, zone.yl);

	_gfxop_add_dirty(state, target);

	if (!pxm) {
		GFXERROR("Attempt to draw_pixmap with pxm=NULL\n");
		return GFX_ERROR;
	}

	_gfxop_scale_rect(&zone, state->driver->mode);
	_gfxop_scale_rect(&target, state->driver->mode);

	return _gfxop_draw_pixmap(state->driver, pxm, -1, -1, zone, target, 
				  gfx_rect(0, 0, 320*state->driver->mode->xfact,
					   200*state->driver->mode->yfact), 0, NULL);
}

int
gfxop_free_pixmap(gfx_state_t *state, gfx_pixmap_t *pxm)
{
	BASIC_CHECKS(GFX_ERROR);
	gfx_free_pixmap(state->driver, pxm);
	return GFX_OK;
}

