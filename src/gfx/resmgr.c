/***************************************************************************
 resmgr.c Copyright (C) 2000 Christoph Reichenbach


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
/* Resource manager core part */


#include <gfx_resource.h>
#include <gfx_tools.h>
#include <gfx_driver.h>
#include <gfx_resmgr.h>
#include <gfx_state_internal.h>

/* Invalid hash mode: Used to invalidate modified pics */
#define MODE_INVALID -1

struct param_struct {
	int args[4];
	gfx_driver_t *driver;
};


gfx_resstate_t *
gfxr_new_resource_manager(int version, gfx_options_t *options,
			  gfx_driver_t *driver, void *misc_payload)
{
	gfx_resstate_t *state = sci_malloc(sizeof(gfx_resstate_t));
	int i;

	state->version = version;
	state->options = options;
	state->driver = driver;
	state->misc_payload = misc_payload;

	state->tag_lock_counter = state->lock_counter = 0;
	for (i = 0; i < GFX_RESOURCE_TYPES_NR; i++) {
		sbtree_t *tree;
		int entries_nr;
		int *resources = gfxr_interpreter_get_resources(state, i, version,
								&entries_nr, misc_payload);

		if (!resources)
			state->resource_trees[i] = NULL;
		else {
			tree = sbtree_new(entries_nr, resources);
			if (!tree) {
				GFXWARN("Failed to allocate tree for %d entries of resource type %d!\n", entries_nr, i);
			}
			state->resource_trees[i] = tree;
			free(resources);
		}
	}
	return state;
}

#define FREEALL(freecmd, type) \
		if (resource->scaled_data.type) \
			freecmd(driver, resource->scaled_data.type); \
                resource->scaled_data.type = NULL; \
		if (resource->unscaled_data.type) \
			freecmd(driver, resource->unscaled_data.type); \
                resource->unscaled_data.type = NULL;

#define FREEALL_SIMPLE(freecmd, type) \
		if (resource->scaled_data.type) \
			freecmd(resource->scaled_data.type); \
                resource->scaled_data.type = NULL; \
		if (resource->unscaled_data.type) \
			freecmd(resource->unscaled_data.type); \
                resource->unscaled_data.type = NULL;


void
gfxr_free_resource(gfx_driver_t *driver, gfx_resource_t *resource, int type)
{
	if (!resource)
		return;

	switch (type) {

	case GFX_RESOURCE_TYPE_VIEW:
		FREEALL(gfxr_free_view, view);
		break;

	case GFX_RESOURCE_TYPE_PIC:
		FREEALL(gfxr_free_pic, pic);
		break;

	case GFX_RESOURCE_TYPE_FONT:
		FREEALL_SIMPLE(gfxr_free_font, font);
		break;

	case GFX_RESOURCE_TYPE_CURSOR:
		FREEALL(gfx_free_pixmap, pointer);
		break;

	default:
		GFXWARN("Attempt to free invalid resource type %d\n", type);
	}

	free(resource);
}

#undef FREECMD


void *
gfxr_sbtree_free_func(sbtree_t *tree, const int key, const void *value, void *args)
{
	struct param_struct *params = (struct param_struct *) args;
	int type = params->args[0];
	gfx_driver_t *driver = params->driver;

	if (value)
		gfxr_free_resource(driver, (gfx_resource_t *) value, type);

	return NULL;
}

#define SBTREE_FREE_TAGGED_ARG_TYPE 0
#define SBTREE_FREE_TAGGED_ARG_TAGVALUE 1
#define SBTREE_FREE_TAGGED_ARG_ACTION 2

#define ARG_ACTION_RESET 0
#define ARG_ACTION_DECREMENT 1

void *
gfxr_sbtree_free_tagged_func(sbtree_t *tree, const int key, const void *value, void *args)
{
	struct param_struct *params = (struct param_struct *) args;
	int type = params->args[SBTREE_FREE_TAGGED_ARG_TYPE];
	int tag_value = params->args[SBTREE_FREE_TAGGED_ARG_TAGVALUE];
	int action = params->args[SBTREE_FREE_TAGGED_ARG_ACTION];
	gfx_driver_t *driver = params->driver;
	gfx_resource_t *resource = (gfx_resource_t *) value;
	if (resource) {
		if (resource->lock_sequence_nr < tag_value) {
			gfxr_free_resource(driver, (gfx_resource_t *) value, type);
			return NULL;
		} else {
			if (action == ARG_ACTION_RESET)
				resource->lock_sequence_nr = 0;
			else
				(resource->lock_sequence_nr)--;
			return (void *) value;
		}
	} else return NULL;
}


void
gfxr_free_all_resources(gfx_driver_t *driver, gfx_resstate_t *state)
{
	struct param_struct params;
	int i;
	sbtree_t *tree = NULL;

	for (i = 0; i < GFX_RESOURCE_TYPES_NR; i++)
		if ((tree = state->resource_trees[i])) {
			params.args[0] = i;
			params.driver = driver;
			sbtree_foreach(tree, (void *) &params, gfxr_sbtree_free_func);
		}
}

void
gfxr_free_resource_manager(gfx_driver_t *driver, gfx_resstate_t *state)
{
	struct param_struct params;
	int i;
	sbtree_t *tree = NULL;

	for (i = 0; i < GFX_RESOURCE_TYPES_NR; i++)
		if ((tree = state->resource_trees[i])) {
			params.args[0] = i;
			params.driver = driver;
			sbtree_foreach(tree, (void *) &params, gfxr_sbtree_free_func);
			sbtree_free(tree);
		}

	free(state);
}


void
gfxr_tag_resources(gfx_resstate_t *state)
{
	(state->tag_lock_counter)++;
}



void
gfxr_free_tagged_resources(gfx_driver_t *driver, gfx_resstate_t *state)
{
	/* Current heuristics: free tagged views and old pics */
	struct param_struct params;

	if (state->resource_trees[GFX_RESOURCE_TYPE_VIEW]) {
		params.args[SBTREE_FREE_TAGGED_ARG_TYPE] = GFX_RESOURCE_TYPE_VIEW;
		params.args[SBTREE_FREE_TAGGED_ARG_TAGVALUE] = state->tag_lock_counter;
		params.args[SBTREE_FREE_TAGGED_ARG_ACTION] = ARG_ACTION_RESET;
		params.driver = driver;
		sbtree_foreach(state->resource_trees[GFX_RESOURCE_TYPE_VIEW], (void *) &params, gfxr_sbtree_free_tagged_func);
	}

	if (state->resource_trees[GFX_RESOURCE_TYPE_PIC]) {
		params.args[SBTREE_FREE_TAGGED_ARG_TYPE] = GFX_RESOURCE_TYPE_PIC;
		params.args[SBTREE_FREE_TAGGED_ARG_TAGVALUE] = 0;
		params.args[SBTREE_FREE_TAGGED_ARG_ACTION] = ARG_ACTION_DECREMENT;
		params.driver = driver;
		sbtree_foreach(state->resource_trees[GFX_RESOURCE_TYPE_PIC], (void *) &params, gfxr_sbtree_free_tagged_func);
	}

	state->tag_lock_counter = 0;
}


#define XLATE_AS_APPROPRIATE(key, entry) \
	if (maps & key) { \
		if (res->unscaled_data.pic \
                    && (force || !res->unscaled_data.pic->entry->data)) \
			        gfx_xlate_pixmap(res->unscaled_data.pic->entry, mode, filter); \
		if (scaled && res->scaled_data.pic \
                    && (force || !res->scaled_data.pic->entry->data)) \
			        gfx_xlate_pixmap(res->scaled_data.pic->entry, mode, filter); \
	}
static gfxr_pic_t *
gfxr_pic_xlate_common(gfx_resource_t *res, int maps, int scaled, int force, gfx_mode_t *mode, int filter, int endianize)
{
	XLATE_AS_APPROPRIATE(GFX_MASK_VISUAL, visual_map);
	XLATE_AS_APPROPRIATE(GFX_MASK_PRIORITY, priority_map);
	XLATE_AS_APPROPRIATE(GFX_MASK_CONTROL, control_map);

        if (endianize && (maps & GFX_MASK_VISUAL) && res->scaled_data.pic->visual_map)
                gfxr_endianness_adjust(res->scaled_data.pic->visual_map, mode);


	return scaled? res->scaled_data.pic : res->unscaled_data.pic;
}
#undef XLATE_AS_APPROPRIATE


gfxr_pic_t *
gfxr_get_pic(gfx_resstate_t *state, int nr, int maps, int flags, int default_palette, int scaled)
{
	gfxr_pic_t *npic = NULL;
	int restype = GFX_RESOURCE_TYPE_PIC;
	sbtree_t *tree = state->resource_trees[restype];
	gfx_resource_t *res = NULL;
	int hash = gfxr_interpreter_options_hash(restype, state->version,
						 state->options, state->misc_payload, 0);
        int must_post_process_pic = 0;
	int need_unscaled =
		(state->driver->mode->xfact != 1 || state->driver->mode->yfact != 1);

	if (!tree)
		return NULL;

	hash |= (flags << 20) | ((default_palette & 0x7) << 28);

	res = (gfx_resource_t *) sbtree_get(tree, nr);

	if (!res || res->mode != hash) {
		gfxr_pic_t *pic;
		gfxr_pic_t *unscaled_pic = NULL;

		if (state->options->pic0_unscaled) {
			need_unscaled = 0;
			pic = gfxr_interpreter_init_pic(state->version,
							&mode_1x1_color_index,
							(restype << 16) | nr,
							state->misc_payload);
		} else pic = gfxr_interpreter_init_pic(state->version,
						       state->driver->mode,
						       (restype << 16) | nr,
						       state->misc_payload);

		if (!pic) {
			GFXERROR("Failed to allocate scaled pic!\n");
			return NULL;
		}

		gfxr_interpreter_clear_pic(state->version, pic, state->misc_payload);

		if (need_unscaled) {
			unscaled_pic = gfxr_interpreter_init_pic(state->version,
								 &mode_1x1_color_index,
								 (restype << 16) | nr,
								 state->misc_payload);
			if (!unscaled_pic) {
				GFXERROR("Failed to allocate unscaled pic!\n");
				return NULL;
			}
			gfxr_interpreter_clear_pic(state->version, unscaled_pic,
						   state->misc_payload);
		}
		if (gfxr_interpreter_calculate_pic(state, pic, unscaled_pic, flags,
						   default_palette, nr,
						   state->misc_payload)) {
			gfxr_free_pic(state->driver, pic);
			if (unscaled_pic)
				gfxr_free_pic(state->driver, unscaled_pic);

			return NULL;
		}

		if (state->options->pic0_unscaled)
			pic->priority_map = gfx_pixmap_scale_index_data(pic->priority_map, state->driver->mode);

		if (!res) {
			res = sci_malloc(sizeof(gfx_resource_t));
			res->ID = ((restype << 16) | nr);
			res->lock_sequence_nr = state->options->buffer_pics_nr;
			sbtree_set(tree, nr, (void *) res);
		} else {
			gfxr_free_pic(state->driver, res->scaled_data.pic);
			if (res->unscaled_data.pic)
				gfxr_free_pic(state->driver, res->unscaled_data.pic);
		}

		res->mode = hash;
		res->scaled_data.pic = pic;
		res->unscaled_data.pic = unscaled_pic;
	} else {
		res->lock_sequence_nr = state->options->buffer_pics_nr; /* Update lock counter */
	}

        must_post_process_pic = res->scaled_data.pic->visual_map->data == NULL;
	/* If the pic was only just drawn, we'll have to antialiase and endianness-adjust it now */

	npic = gfxr_pic_xlate_common(res, maps,
				    scaled || state->options->pic0_unscaled,
				    0, state->driver->mode,
				    state->options->pic_xlate_filter, 0);


	if (must_post_process_pic) {

		if (scaled || state->options->pic0_unscaled && maps & GFX_MASK_VISUAL)
                        gfxr_antialiase(npic->visual_map, state->driver->mode,
                                        state->options->pic0_antialiasing);

                gfxr_endianness_adjust(npic->visual_map, state->driver->mode);
	}

	return npic;
}


gfxr_pic_t *
gfxr_add_to_pic(gfx_resstate_t *state, int old_nr, int new_nr, int maps, int flags,
		int old_default_palette, int default_palette, int scaled)
{
	int restype = GFX_RESOURCE_TYPE_PIC;
	sbtree_t *tree = state->resource_trees[restype];
	gfxr_pic_t *pic = NULL;
	gfx_resource_t *res = NULL;
	int hash = gfxr_interpreter_options_hash(restype, state->version,
						 state->options,
						 state->misc_payload, 0);
	int need_unscaled = !(state->options->pic0_unscaled)
		&& (state->driver->mode->xfact != 1 || state->driver->mode->yfact != 1);

	if (!tree) {
		GFXERROR("No pics registered\n");
		return NULL;
	}

	res = (gfx_resource_t *) sbtree_get(tree, old_nr);

	if (!res ||
	    (res->mode != MODE_INVALID
	     && res->mode != hash)) {
		gfxr_get_pic(state, old_nr, 0, flags, old_default_palette, scaled);

		res = (gfx_resource_t *) sbtree_get(tree, old_nr);

		if (!res) {
			GFXWARN("Attempt to add pic %d to non-existing pic %d\n", new_nr, old_nr);
			return NULL;
		}
	}

	if (scaled) {
		res->lock_sequence_nr = state->options->buffer_pics_nr;

		gfxr_interpreter_calculate_pic(state, res->scaled_data.pic, need_unscaled? res->unscaled_data.pic : NULL,
					       flags, default_palette, new_nr,
					       state->misc_payload);
	}

	res->mode = MODE_INVALID; /* Invalidate */

	pic = gfxr_pic_xlate_common(res, maps, scaled, 1, state->driver->mode, state->options->pic_xlate_filter, 1);

	if (scaled || state->options->pic0_unscaled && maps & GFX_MASK_VISUAL)
		gfxr_antialiase(pic->visual_map, state->driver->mode,
				state->options->pic0_antialiasing);

	return pic;
}


gfxr_view_t *
gfxr_get_view(gfx_resstate_t *state, int nr, int *loop, int *cel, int palette)
{
	int restype = GFX_RESOURCE_TYPE_VIEW;
	sbtree_t *tree = state->resource_trees[restype];
	gfx_resource_t *res = NULL;
	int hash = gfxr_interpreter_options_hash(restype, state->version,
						 state->options, state->misc_payload,
						 palette);
	gfxr_view_t *view = NULL;
	gfxr_loop_t *loop_data = NULL;
	gfx_pixmap_t *cel_data = NULL;

	if (!tree)
		return NULL;

	res = (gfx_resource_t *) sbtree_get(tree, nr);

	if (!res || res->mode != hash) {
		view = gfxr_interpreter_get_view(state, nr, state->misc_payload, palette);

		if (!view)
			return NULL;

		if (!res) {
			res = sci_malloc(sizeof(gfx_resource_t));
			res->scaled_data.view = NULL;
			res->ID = ((restype << 16) | nr);
			res->lock_sequence_nr = state->tag_lock_counter;
			res->mode = hash;
			sbtree_set(tree, nr, (void *) res);
		} else {
			gfxr_free_view(state->driver, res->unscaled_data.view);
		}

		res->mode = hash;
		res->unscaled_data.view = view;

	} else {
		res->lock_sequence_nr = state->tag_lock_counter; /* Update lock counter */
		view = res->unscaled_data.view;
	}

	if (*loop < 0)
		*loop = 0;
	else
		if (*loop >= view->loops_nr)
			*loop = view->loops_nr - 1;

	loop_data = view->loops + (*loop);

	if (*cel < 0)
		*cel = 0;
	else
		if (*cel >= loop_data->cels_nr)
			*cel = loop_data->cels_nr - 1;

	cel_data = loop_data->cels[*cel];

	if (!cel_data->data) {
		gfx_xlate_pixmap(cel_data, state->driver->mode, state->options->view_xlate_filter);
                gfxr_endianness_adjust(cel_data, state->driver->mode);
        }

	return view;
}


gfx_bitmap_font_t *
gfxr_get_font(gfx_resstate_t *state, int nr, int scaled)
{
	int restype = GFX_RESOURCE_TYPE_FONT;
	sbtree_t *tree = state->resource_trees[restype];
	gfx_resource_t *res = NULL;
	int hash = gfxr_interpreter_options_hash(restype, state->version,
						 state->options, state->misc_payload, 0);

	if (!tree)
		return NULL;

	res = (gfx_resource_t *) sbtree_get(tree, nr);

	if (!res || res->mode != hash) {
		gfx_bitmap_font_t *font = gfxr_interpreter_get_font(state, nr,
								    state->misc_payload);

		if (!font)
			return NULL;

		if (!res) {
			res = sci_malloc(sizeof(gfx_resource_t));
			res->scaled_data.font = NULL;
			res->ID = ((restype << 16) | nr);
			res->lock_sequence_nr = state->tag_lock_counter;
			res->mode = hash;
			sbtree_set(tree, nr, (void *) res);
		} else {
			gfxr_free_font(res->unscaled_data.font);
		}

		res->unscaled_data.font = font;

		return font;
	} else {
		res->lock_sequence_nr = state->tag_lock_counter; /* Update lock counter */
		if (res->unscaled_data.pointer)
			return res->unscaled_data.font;
		else
			return res->scaled_data.font;
	}
}


gfx_pixmap_t *
gfxr_get_cursor(gfx_resstate_t *state, int nr)
{
	int restype = GFX_RESOURCE_TYPE_CURSOR;
	sbtree_t *tree = state->resource_trees[restype];
	gfx_resource_t *res = NULL;
	int hash = gfxr_interpreter_options_hash(restype, state->version,
						 state->options, state->misc_payload, 0);

	if (!tree)
		return NULL;

	res = (gfx_resource_t *) sbtree_get(tree, nr);

	if (!res || res->mode != hash) {
		gfx_pixmap_t *cursor = gfxr_interpreter_get_cursor(state, nr,
								   state->misc_payload);

		if (!cursor)
			return NULL;

		if (!res) {
			res = sci_malloc(sizeof(gfx_resource_t));
			res->scaled_data.pointer = NULL;
			res->ID = ((restype << 16) | nr);
			res->lock_sequence_nr = state->tag_lock_counter;
			res->mode = hash;
			sbtree_set(tree, nr, (void *) res);
		} else {
			gfx_free_pixmap(state->driver, res->unscaled_data.pointer);
		}
		gfx_xlate_pixmap(cursor, state->driver->mode, state->options->cursor_xlate_filter);
                gfxr_endianness_adjust(cursor, state->driver->mode);

		res->unscaled_data.pointer = cursor;

		return cursor;
	} else {
		res->lock_sequence_nr = state->tag_lock_counter; /* Update lock counter */
		return res->unscaled_data.pointer;
	}
}
