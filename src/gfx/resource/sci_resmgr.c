/***************************************************************************
 sci_resmgr.c Copyright (C) 2000 Christoph Reichenbach

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
/* The interpreter-specific part of the resource manager, for SCI */


#include <resource.h>
#include <gfx_widgets.h>
#include <gfx_resmgr.h>
#include <gfx_options.h>

int
gfxr_interpreter_options_hash(gfx_resource_types_t type, int version, gfx_options_t *options)
{
	switch (type) {

	case GFX_RESOURCE_TYPE_VIEW:
		return 0;

	case GFX_RESOURCE_TYPE_PIC:
		if (version >= SCI_VERSION_1)
			return 0;
		else
			return (options->pic0_unscaled)? 0x10000000 :
				(options->pic0_dither_mode << 24)
				| (options->pic0_dither_pattern << 16)
				| (options->pic0_brush_mode << 8)
				| (options->pic0_line_mode);

	case GFX_RESOURCE_TYPE_FONT:
		return 0;

	case GFX_RESOURCE_TYPE_CURSOR:
		return 0;

	default:
		GFXERROR("Invalid resource type: %d\n", type);
		return -1;
	}
}


gfxr_pic_t *
gfxr_interpreter_init_pic(int version, gfx_mode_t *mode, int ID)
{
	return gfxr_init_pic(mode, ID);
}


void
gfxr_interpreter_clear_pic(int version, gfxr_pic_t *pic)
{
	gfxr_clear_pic0(pic);
}


int
gfxr_interpreter_calculate_pic(gfx_resstate_t *state, gfxr_pic_t *scaled_pic, gfxr_pic_t *unscaled_pic,
			       int flags, int default_palette, int nr)
{
	resource_t *res = findResource(sci_pic, nr);
	int need_unscaled = unscaled_pic != NULL;
	gfxr_pic0_params_t style, basic_style;

	basic_style.line_mode = GFX_LINE_MODE_CORRECT;
	basic_style.brush_mode = GFX_BRUSH_MODE_SCALED;

	style.line_mode = state->options->pic0_line_mode; 
	style.brush_mode = state->options->pic0_brush_mode; 

	if (!res || !res->data)
		return GFX_ERROR;

	if (state->version >= SCI_VERSION_1) {
		GFXWARN("Attempt to retreive pic in SCI1 or later\n");
		return GFX_ERROR;
	}

	if (need_unscaled)
		gfxr_draw_pic0(unscaled_pic, flags, default_palette, res->length, res->data, &basic_style, res->id);

	gfxr_draw_pic0(scaled_pic, flags, default_palette, res->length, res->data, &basic_style, res->id);
	if (need_unscaled)
		gfxr_remove_artifacts_pic0(scaled_pic, unscaled_pic);

	gfxr_dither_pic0(scaled_pic, state->options->pic0_dither_mode, state->options->pic0_dither_pattern);

	return GFX_OK;
}


gfxr_view_t *
gfxr_interpreter_get_view(gfx_resstate_t *state, int nr)
{
	resource_t *res = findResource(sci_view, nr);

	if (!res || !res->data)
		return NULL;

	if (state->version >= SCI_VERSION_1) {
		GFXWARN("Attempt to retreive view in SCI1 or later\n");
		return NULL;
	}

	return gfxr_draw_view0(res->id, res->data, res->length);
}


gfx_bitmap_font_t *
gfxr_interpreter_get_font(gfx_resstate_t *state, int nr)
{
	resource_t *res = findResource(sci_font, nr);

	if (!res || !res->data)
		return NULL;

	return gfxr_read_font(res->id, res->data, res->length);
}


gfx_pixmap_t *
gfxr_interpreter_get_cursor(gfx_resstate_t *state, int nr)
{
	resource_t *res = findResource(sci_cursor, nr);

	if (!res || !res->data)
		return NULL;

	if (state->version >= SCI_VERSION_1) {
		GFXWARN("Attempt to retreive cursor in SCI1 or later\n");
		return NULL;
	}

	if (state->version == SCI_VERSION_0)
		return gfxr_draw_cursor0(res->id, res->data, res->length);
	else
		return gfxr_draw_cursor01(res->id, res->data, res->length);
}


int *
gfxr_interpreter_get_resources(gfx_resource_types_t type, int version, int *entries_nr)
{
	int restype;
	int *resources;
	int count = 0;
	int top = sci_max_resource_nr[version] + 1;
	int i;

	switch (type) {

	case GFX_RESOURCE_TYPE_VIEW: restype = sci_view;
		break;

	case GFX_RESOURCE_TYPE_PIC: restype = sci_pic;
		break;

	case GFX_RESOURCE_TYPE_CURSOR: restype = sci_cursor;
		break;

	case GFX_RESOURCE_TYPE_FONT: restype = sci_font;
		break;

	default:
		GFX_DEBUG("Unsupported resource %d\n", type);
		return NULL; /* unsupported resource */

	}

	resources = malloc(sizeof(int) * top);

	for (i = 0; i < top; i++)
		if (findResource(restype, i))
			resources[count++] = i;

	*entries_nr = count;

	return resources;
}

gfx_pixmap_color_t *
gfxr_interpreter_get_palette(int version, int *colors_nr)
{
	if (version >= SCI_VERSION_1)
		return NULL;

	*colors_nr = GFX_SCI0_PIC_COLORS_NR;
	return gfx_sci0_pic_colors;
}


int
gfxr_interpreter_needs_multicolored_pointers(int version)
{
	return (version > SCI_VERSION_0);
}


