/***************************************************************************
 midiout.h Copyright (C) 2000 Rickard Lind

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

***************************************************************************/

#ifndef _MIDIOUT_H_
#define _MIDIOUT_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <sci_memory.h>
#include <resource.h>

typedef struct _midiout_driver {
	char *name;
	char *version;
	int (*set_parameter)(struct _midiout_driver *drv, char *attribute, char *value);
	int (*midiout_open)();
	int (*midiout_close)();
	int (*midiout_write)(guint8 *data, unsigned int count, guint32 delta);
	int (*midiout_flush)(guint8);
} midiout_driver_t;

extern DLLEXTERN midiout_driver_t *midiout_driver;

extern DLLEXTERN midiout_driver_t midiout_driver_null;

#if !defined(_DOS) && !defined(WIN32)
extern midiout_driver_t midiout_driver_unixraw;
#endif

#ifdef HAVE_ALSA
extern midiout_driver_t midiout_driver_alsaraw;
#endif

#ifdef HAVE_DMEDIA_MIDI_H
extern midiout_driver_t midiout_driver_sgimd;
#endif

#ifdef HAVE_SYS_SOUNDCARD_H
extern midiout_driver_t midiout_driver_ossseq;
extern midiout_driver_t midiout_driver_ossopl3;
#endif

#ifdef __MORPHOS__
extern midiout_driver_t midiout_driver_morphos;
#endif

#ifdef _WIN32
extern midiout_driver_t midiout_driver_win32mci;
extern midiout_driver_t midiout_driver_win32mci_stream;
#endif

#ifdef HAVE_DMEDIA_MIDI_H
extern midiout_driver_t midiout_driver_sgimd;
#endif

#ifdef HAVE_FLUIDSYNTH_H
extern midiout_driver_t midiout_driver_fluidsynth;
#endif

#ifdef _DREAMCAST
extern midiout_driver_t midiout_driver_dcraw;
#endif

extern DLLEXTERN midiout_driver_t *midiout_drivers[];

int midiout_open();
int midiout_close();
int midiout_write_event(guint8 *buffer, unsigned int count, guint32 delta);
int midiout_write_block(guint8 *buffer, unsigned int count, guint32 delta);
int midiout_flush(guint8);

struct _midiout_driver *midiout_find_driver(char *name);

#endif /* _MIDIOUT_H_ */
