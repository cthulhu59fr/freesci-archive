/***************************************************************************
 engine_graphics.c Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* Graphics operations that are relative to a state_t */


#include <engine.h>


typedef struct {
  int x, y, xl, yl;
  int layers;

  byte data[1];
} _graph_memrect_t;


void
graph_clear_box(struct _state *s, int x, int y, int xl, int yl, int color)
{
  int pos = y * SCI_SCREEN_WIDTH + x;
  int _yl = yl;

  while (_yl--) {
    memset(s->pic[0] + pos, color, xl);

    pos += SCI_SCREEN_WIDTH;
  }

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_BOX, x, y, xl, yl);
}


void
graph_update_box(struct _state *s, int x, int y, int xl, int yl)
{
  int pos = y * SCI_SCREEN_WIDTH + x;
  int i, _yl = yl;

  while (_yl--) {
    for (i=0; i < 3; i++)
      memcpy(s->pic[i] + pos, s->bgpic[i] + pos, xl);

    pos += SCI_SCREEN_WIDTH;
  }

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_BOX, x, y, xl, yl);
}

int
graph_save_box(struct _state *s, int x, int y, int xl, int yl, int layers)
{
  int handle = kalloc(s, xl * yl * 3 +  5*(sizeof(int))); /* Three layers plus x, y, xl, yl, layers */
  _graph_memrect_t *box = (_graph_memrect_t *) kmem(s, handle);
  byte *dest = &(box->data[0]);
  int i, map;
  int pos;

  box->x = x;
  box->y = y;
  box->xl = xl;
  box->yl = yl;
  box->layers = layers;

  for (map = 0; map < 3; map++)
    if (layers & (1 << map)) {

      pos = x + y * SCI_SCREEN_WIDTH;

      for (i = yl; i > 0; i--) {
	memcpy(dest, s->bgpic[map] + pos, xl);

	pos += SCI_SCREEN_WIDTH;
	dest += xl;

      }
    }

  return handle;
}

void
graph_restore_box(struct _state *s, int handle)
{
  _graph_memrect_t *box;
  byte *src;
  int map, pos;
  int i;

  if (!handle)
    return; /* Assume that the caller knew that this wouldn't work */

  box = (_graph_memrect_t *) kmem(s, handle);
  if (!box) {
    sciprintf("graph_restore_box: Warning: Could not restore handle %04x\n", handle);
    return;
  }

  src = &(box->data[0]);

  for (map = 0; map < 3; map++)
    if (box->layers & (1 << map)) {

      pos = box->x + box->y * SCI_SCREEN_WIDTH;

      for (i = box->yl; i > 0; i--) {
	memcpy(s->bgpic[map] + pos, src, box->xl);

	pos += SCI_SCREEN_WIDTH;
	src += box->xl;

      }
    }

  s->graphics_callback(s, GRAPHICS_CALLBACK_REDRAW_BOX, box->x, box->y, box->xl, box->yl);
  kfree(s, handle);
}


void
graph_fill_port(struct _state *s, port_t *port, int color)
{
  int pos = port->ymin * SCI_SCREEN_WIDTH + port->xmin;
  int _yl = port->ymax - port->ymin + 1;

  if (color < 0)
    return;

  while (_yl--) {
    memset(s->bgpic[0] + pos, color, port->xmax - port->xmin + 1);
    pos += SCI_SCREEN_WIDTH;
  }
}