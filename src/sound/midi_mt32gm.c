/***************************************************************************
 midi_mt32.c Copyright (C) 2001 Solomon Peachy

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

#include <midi_device.h>
#include <sound.h>

/* we reuse the mt32 open/close/etc */

int midi_mt32_event(guint8 command, guint8 param, guint8 param2);
int midi_mt32_event2(guint8 command, guint8 param);
int midi_mt32_allstop(void);

/* gm mapping of mt-32 */

int midi_mt32gm_open(guint8 *data_ptr, unsigned int data_length)
{
  if (midiout_open() < 0)
    return -1;
  return midi_mt32_allstop();
}

int midi_mt32gm_close()
{
  return midiout_close();
}


int midi_mt32gm_event(guint8 command, guint8 param, guint8 param2)
{
  guint8 channel;
  guint8 oper;

  channel = command & 0x0f;
  oper = command & 0xf0;

  switch (oper) {
  case 0x90:
  case 0x80:  /* noteon and noteoff */
    param2 = MIDI_mapping[param].volume;
    if (channel == RHYTHM_CHANNEL)
      param = MIDI_mapping[param].gm_rhythmkey;
    else
      param = MIDI_mapping[param].gm_instr;
    break;
  case 0xe0:    /* Pitch bend NYI */
    break;
  case 0xb0:    /* CC changes.  let 'em through... */
    break;
  default:
    printf("MT32GM: Unknown event: %02x\n", command);

  }

  return midi_mt32_event(command, param, param2);
}

int midi_mt32gm_event2(guint8 command, guint8 param)
{
  guint8 channel;
  guint8 oper;

  channel = command & 0x0f;
  oper = command & 0xf0;
  switch (oper) {
  case 0xc0:  /* change instrument */
    if (channel == RHYTHM_CHANNEL)
      param = MIDI_mapping[param].gm_rhythmkey;
    else
      param = MIDI_mapping[param].gm_instr;
    break;
  default:
    printf("MT32GM: Unknown event: %02x\n", command);
  }

  return midi_mt32_event2(command, param);
}

int midi_mt32gm_volume(guint8 volume)
{
  printf("MT32GM: Volume control not supported\n");
  return 0;
}

int midi_mt32gm_reverb(short param)
{
  printf("MT32GM: Reverb control not supported\n");
  return 0;
}

/* device struct */

midi_device_t midi_device_mt32gm = {
  "mt32gm",
  "v0.01",
  &midi_mt32gm_open,
  &midi_mt32gm_close,
  &midi_mt32gm_event,
  &midi_mt32gm_event2,
  &midi_mt32_allstop,
  &midi_mt32gm_volume,
  &midi_mt32gm_reverb,
  001,		/* patch.001 */
  0x01,		/* playflag */
  1, 		/* play channel 9 */
  32		/* Max polyphony */
};