/***************************************************************************
 midi_adlibemu.c Copyright (C) 2002 Solomon Peachy

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

#include <stdio.h>
#ifdef HAVE_UNISTD_H
  #include <unistd.h>
#endif
#include <string.h>
#include <midi_device.h>
#include <midiout.h>
#include <soundserver.h>

#include "fmopl.h"

/* shamelessly lifted from claudio's XMP */

static int register_base[11] = {
    0x20, 0x20, 0x40, 0x40,
    0x60, 0x60, 0x80, 0x80,
    0xe0, 0xe0, 0xc0
};

static int register_offset[2][9] = {
    /* Channel           1     2     3     4     5     6     7     8     9 */
    /* Operator 1 */ { 0x00, 0x01, 0x02, 0x08, 0x09, 0x0A, 0x10, 0x11, 0x12 },
    /* Operator 2 */ { 0x03, 0x04, 0x05, 0x0B, 0x0C, 0x0D, 0x13, 0x14, 0x15 }
}; 

static int ym3812_note[13] = {
    0x157, 0x16b, 0x181, 0x198, 0x1b0, 0x1ca,
    0x1e5, 0x202, 0x220, 0x241, 0x263, 0x287,
    0x2ae
};

/* logarithmic relationship between midi and FM volumes */
static int my_midi_fm_vol_table[128] = {
   0,  11, 16, 19, 22, 25, 27, 29, 32, 33, 35, 37, 39, 40, 42, 43,
   45, 46, 48, 49, 50, 51, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
   64, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 75, 76, 77,
   78, 79, 80, 80, 81, 82, 83, 83, 84, 85, 86, 86, 87, 88, 89, 89,
   90, 91, 91, 92, 93, 93, 94, 95, 96, 96, 97, 97, 98, 99, 99, 100,
   101, 101, 102, 103, 103, 104, 104, 105, 106, 106, 107, 107, 108,
   109, 109, 110, 110, 111, 112, 112, 113, 113, 114, 114, 115, 115,
   116, 117, 117, 118, 118, 119, 119, 120, 120, 121, 121, 122, 122,
   123, 123, 124, 124, 125, 125, 126, 126, 127
};

/* back to your regularly scheduled definitions */

static guint8 instr[MIDI_CHANNELS];
static guint8 pitch[MIDI_CHANNELS];
static guint8 vol[MIDI_CHANNELS];
static guint8 adlib_reg[256];
static int dev;
static int free_voices = ADLIB_VOICES;
static unsigned char oper_note[ADLIB_VOICES];
static unsigned char oper_chn[ADLIB_VOICES];

static FM_OPL *ym3812 = NULL;

/* initialise note/operator lists, etc. */
void adlibemu_init_lists()
{
  int i;

  for(i = 0 ; i < ADLIB_VOICES ; i++) {
    oper_note[i] = 255;
    oper_chn[i] = 255;
  }

  memset(instr, 0, sizeof(instr));
  memset(pitch, 0, sizeof(instr));
  memset(vol, 0x7f, sizeof(instr));
  memset(adlib_reg, 0, sizeof(instr));
}

/* more shamelessly lifted from xmp and adplug.  And altered.  :) */

static inline int opl_write (FM_OPL *opl, int a, int v)
{
  adlib_reg[a] = v;
  OPLWrite (opl, 0x388, a);
  return OPLWrite (opl, 0x389, v);
}

static inline guint8 opl_read (FM_OPL *opl, int a)
{
  OPLWrite (opl, 0x388, a);
  return OPLRead (opl, 0x389);
}

void synth_setpatch (int voice, guint8 *data)
{
  int i, x;
  
  for (i = 0; i < 10; i++)
    opl_write (ym3812, register_base[i] + register_offset[i % 2][voice], data[i]); 
  opl_write (ym3812, register_base[10] + voice, data[10]);

  /* mute voice after patch change */
  opl_write (ym3812, 0xb0 + voice, adlib_reg[0xb0+voice] & 0xdf);
}

void synth_setvolume (int voice, int volume)
{
  volume = volume >> 1;  /* adlib is 6-bit, midi is 7-bit */
  
  /* algorithm-dependent; we may need to set both operators. */
  if (adlib_reg[register_base[10]+voice] & 1)
    opl_write(ym3812, register_base[2]+register_offset[0][voice], 
	      ((63-volume) | 
	       (adlib_reg[register_base[2]+register_offset[0][voice]]&0xc0)));
  
  opl_write(ym3812, register_base[3]+register_offset[1][voice],
	    ((63-volume) |
	     (adlib_reg[register_base[3]+register_offset[1][voice]]&0xc0)));

}

void synth_setnote (int voice, int note, int bend)
{
    int n, fre, oct;

    n = note % 12;
    fre = ym3812_note[n] + (ym3812_note[n + 1] - ym3812_note[n]) * bend / 100;
    oct = note / 12 - 1;

    if (oct < 0)
        oct = 0;

    opl_write (ym3812, 0xa0 + voice, fre & 0xff);
    opl_write (ym3812, 0xb0 + voice,
        0x20 | ((oct << 2) & 0x1c) | ((fre >> 8) & 0x03));
}


/* back to your regularly scheduled driver */

int adlibemu_stop_note(int chn, int note, int velocity)
{
  int i, op=255;
  
  for (i=0;i<ADLIB_VOICES && op==255;i++) {
    if (oper_chn[i] == chn)
      if (oper_note[i] == note)
	op=i;
  }

  if (op==255) {
    /* printf ("can't stop.. chn %d %d %d\n", chn, note, velocity); */
    return -1; /* that note isn't playing.. */
  }

  opl_write(ym3812, 0xb0+op,(adlib_reg[0xb0+op] & 0xdf)
);

  /*   synth_setnote(op, note, pitch[chn]);  */

  oper_chn[op] = 255;
  oper_note[op] = 255;
  
  free_voices++;

  return op;
}

int adlibemu_start_note(int chn, int note, int velocity)
{
  int op = 0;
  int volume;

  if (velocity == 0) {
    adlibemu_stop_note(chn, note, velocity);
    return;
  }

  if (free_voices <= 0) {
    printf("ack, all voices full\n");  /* XXX implement overflow code */
    return;
  } else 
    for (op = 0; op < ADLIB_VOICES ; op++)
      if (oper_chn[op] == 255)
	break;

  volume = velocity * vol[chn] / 128; /* Scale channel volume */
  volume = my_midi_fm_vol_table[volume];

  synth_setpatch(op, &(instr[chn]));
  synth_setvolume(op, volume);
  synth_setnote(op, note, pitch[chn]); 

  printf("play operator %d/%d:  C%02x N%02x V%02x (%02x/%02x)\n", op, free_voices, chn, note, velocity, 
	 adlib_reg[register_base[2]+register_offset[0][op]],
	 adlib_reg[register_base[3]+register_offset[1][op]]);

  oper_chn[op] = chn;
  oper_note[op] = note;
  free_voices--;
  

  return;
}

int midi_adlibemu_open(guint8 *data_ptr, unsigned int data_length)
{
  int nrdevs, i, n;

  /* load up the patch.003 file, parse out the insturments */
  if (data_length < 1344) {
    printf ("invalid patch.003");
    return -1;
  }

  for (i = 0; i < 48; i++) 
    make_sbi((adlib_def *)(data_ptr+(28 * i)), adlib_sbi[i]);

  if (data_length > 1344)
    for (i = 48; i < 96; i++) 
      make_sbi((adlib_def *)(data_ptr+2+(28 * i)), adlib_sbi[i]);

  ym3812 = OPLCreate (OPL_TYPE_YM3812, 3579545, 44100);

  // XXX register with pcm layer.

  return midi_adlibemu_reset();
}


int midi_adlibemu_close()
{
  OPLDestroy (ym3812);
  // XXX deregister with pcm layer.
  return 0;
}

int midi_adlibemu_volume(guint8 volume)
{
  volume &= 0x7f; /* (make sure it's not over 127) */

  printf("volume NYI %02x \n", volume);
  return 0;

}

int midi_adlibemu_reset(void) {
  if (ym3812 == NULL)
    return -1;

  adlibemu_init_lists();
  OPLResetChip (ym3812);
  opl_write(ym3812, 0x01, 0x20);
  opl_write(ym3812, 0xBD, 0xc0);
  return 0;
}

int midi_adlibemu_reverb(short param)
{
  printf("reverb NYI %04x \n", param);
  return 0;
}

int midi_adlibemu_event(guint8 command, guint8 note, guint8 velocity)
{
  guint8 channel, oper;

  channel = command & 0x0f;
  oper = command & 0xf0;

  switch (oper) {    
  case 0x80:
    adlibemu_stop_note(channel, note, velocity);
    return 0;
  case 0x90:  /* noteon and noteoff */
    adlibemu_start_note(channel,note,velocity);
    return 0;
  case 0xe0:    /* Pitch bend NYI */
    break;
  case 0xb0:    /* CC changes. */
    switch (note) {
    case 0x07:
      vol[channel] = velocity;
      break;
    default:
      /* XXXX ignore everything else for now */
    }
    return 0;
  case 0xd0:    /* aftertouch */
    /* XXX Aftertouch in the OPL thing? */
    return 0;
  default:
    printf("ADLIB: Unknown event %02x\n", command);
    return 0;
  }
  
  return 0;
}

int midi_adlibemu_event2(guint8 command, guint8 param)
{
  guint8 channel;
  guint8 oper;
  int xparam = param;
  
  channel = command & 0x0f;
  oper = command & 0xf0;
  switch (oper) {
  case 0xc0: {  /* change instrument */
    int inst = param;
    instr[channel] = inst;
    return 0;
  }
  default:
    printf("ADLIB: Unknown event %02x\n", command);
  }

  return 0;
}

/* the driver struct */

midi_device_t midi_device_adlibemu = {
  "adlibemu",
  "v0.01",
  &midi_adlibemu_open,
  &midi_adlibemu_close,
  &midi_adlibemu_event,
  &midi_adlibemu_event2,
  &midi_adlibemu_reset,
  &midi_adlibemu_volume,
  &midi_adlibemu_reverb,
  003,		/* patch.003 */
  0x04,		/* playflag */
  1, 		/* play channel 9 -- rhythm ?notnot */
  ADLIB_VOICES  /* Max polyphony */
};

void synth_mixer (void* buffer, int count)
{
    if (!buffer)
      return;
    if (!ym3812)  /* if either is uninitialized, bad things happen */
      return;

    YM3812UpdateOne (ym3812, buffer, count);
}
