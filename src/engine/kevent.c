/***************************************************************************
 kevent.c Copyright (C) 1999 Christoph Reichenbach


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

#include <engine.h>



void
kGetEvent(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  int mask = UPARAM(0);
  heap_ptr obj = UPARAM(1);
  sci_event_t e;
  int oldx, oldy;
  CHECK_THIS_KERNEL_FUNCTION;
  
  /*If there's a simkey pending, and the game wants a keyboard event, use the
   *simkey instead of a normal event*/
  if (_kdebug_cheap_event_hack && (mask&SCI_EVT_KEYBOARD)) {
    PUT_SELECTOR(obj, type, SCI_EVT_KEYBOARD); /*Keyboard event*/
    s->acc=1;
    PUT_SELECTOR(obj, message, _kdebug_cheap_event_hack);
    PUT_SELECTOR(obj, modifiers, SCI_EVM_NUMLOCK); /*Numlock on*/
    PUT_SELECTOR(obj, x, s->pointer_x);
    PUT_SELECTOR(obj, y, s->pointer_y);
    _kdebug_cheap_event_hack = 0;
    return;
  }
  
  oldx=s->pointer_x;
  oldy=s->pointer_y;
  e=getEvent(s);

  PUT_SELECTOR(obj, x, s->pointer_x);
  PUT_SELECTOR(obj, y, s->pointer_y);
  if((oldx!=s->pointer_x || oldy!=s->pointer_y) && s->have_mouse_flag)
    s->gfx_driver->Redraw(s, GRAPHICS_CALLBACK_REDRAW_POINTER, 0, 0, 0, 0);
  
  switch(e.type)
    {
    case SCI_EVT_KEYBOARD:
      {
	if ((e.buckybits & SCI_EVM_LSHIFT) && (e.buckybits & SCI_EVM_RSHIFT)
	    && (e.data == '-')) {

	  sciprintf("Debug mode activated\n");

	  script_debug_flag = 1; /* Enter debug mode */
	  _debug_seeking = _debug_step_running = 0;
	  s->onscreen_console = 0;

	} else

	  if ((e.buckybits & SCI_EVM_CTRL) && (e.data == '`')) {

	    script_debug_flag = 1; /* Enter debug mode */
	    _debug_seeking = _debug_step_running = 0;
	    s->onscreen_console = 1;

	} else {

	  PUT_SELECTOR(obj, type, SCI_EVT_KEYBOARD); /*Keyboard event*/
	  s->acc=1;
	  PUT_SELECTOR(obj, message, e.data);
	  PUT_SELECTOR(obj, modifiers, e.buckybits);
	}
      } break;
    case SCI_EVT_MOUSE_RELEASE:
    case SCI_EVT_MOUSE_PRESS:
      {
	int extra_bits=0;
	if(mask&e.type)
          {
            switch(e.data)
	      {
	      case 2: extra_bits=SCI_EVM_LSHIFT|SCI_EVM_RSHIFT; break;
	      case 3: extra_bits=SCI_EVM_CTRL;
	      }
	    PUT_SELECTOR(obj, type, e.type);
	    PUT_SELECTOR(obj, message, 1);
	    PUT_SELECTOR(obj, modifiers, e.buckybits|extra_bits);
	    s->acc=1;
	  }
	return;
      } break;
    default:
      {
	s->acc = 0; /* Unknown or no event */
      }
    }
}

void
kMapKeyToDir(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = UPARAM(0);

  if (GET_SELECTOR(obj, type) == SCI_EVT_KEYBOARD) { /* Keyboard */
    int mover = -1;
    switch (GET_SELECTOR(obj, message)) {
    case SCI_K_HOME: mover = 8; break;
    case SCI_K_UP: mover = 1; break;
    case SCI_K_PGUP: mover = 2; break;
    case SCI_K_LEFT: mover = 7; break;
    case SCI_K_CENTER:
    case 76: mover = 0; break;
    case SCI_K_RIGHT: mover = 3; break;
    case SCI_K_END: mover = 6; break;
    case SCI_K_DOWN: mover = 5; break;
    case SCI_K_PGDOWN: mover = 4; break;
    }

    if (mover >= 0) {
      PUT_SELECTOR(obj, type, SCI_EVT_JOYSTICK);
      PUT_SELECTOR(obj, message, mover);
      s->acc = 1;
    } else s->acc = 0;
  }
}


void
kGlobalToLocal(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = UPARAM_OR_ALT(0, 0);

  if (obj) {
    int x = GET_SELECTOR(obj, x);
    int y = GET_SELECTOR(obj, y);

    PUT_SELECTOR(obj, x, x - s->ports[s->view_port]->xmin);
    PUT_SELECTOR(obj, y, y - s->ports[s->view_port]->ymin);
  }
}


void
kLocalToGlobal(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  heap_ptr obj = UPARAM_OR_ALT(0, 0);

  if (obj) {
    int x = GET_SELECTOR(obj, x);
    int y = GET_SELECTOR(obj, y);

    PUT_SELECTOR(obj, x, x + s->ports[s->view_port]->xmin);
    PUT_SELECTOR(obj, y, y + s->ports[s->view_port]->ymin);
  }
}

void /* Not implemented */
kJoystick(state_t *s, int funct_nr, int argc, heap_ptr argp)
{
  CHECK_THIS_KERNEL_FUNCTION;
  SCIkwarn(SCIkSTUB, "Unimplemented syscall 'Joystick()'\n", funct_nr);
  s->acc = 0;
}

