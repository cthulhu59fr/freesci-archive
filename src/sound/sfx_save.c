/***************************************************************************
 sfx_save.cfsml Copyright (C) 1999 Christoph Reichenbach, TU Darmstadt


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
/* CFSML file providing persistance to the sound system
** Note that this is only useful if the default sound library implementation
** is in use.
*/

#include <stdio.h>
#include <sound.h>
#include <soundserver.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif  /* !O_BINARY */

typedef struct {

  int songs_nr; /* Number of songs */
  song_t *songs; /* All songs in order */

  int active_song;

  int soundcue; /* Cumulative sound vue */
  int usecs_to_sleep; /* Microseconds until the next tick is due */
  int ticks_to_wait; /* Ticks until the next sound command has to be interpreted */
  int ticks_to_fade; /* Ticks until a fade-out is complete */

} sound_lib_file_t;




/* Auto-generated CFSML declaration and function block */

#line 599 "cfsml.pl"
#define CFSML_SUCCESS 0
#define CFSML_FAILURE 1

#line 52 "cfsml.pl"

#include <stdarg.h> /* We need va_lists */

static void
_cfsml_error(char *fmt, ...)
{
  va_list argp;

  fprintf(stderr, "Error: ");
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);

}

static char *
_cfsml_mangle_string(char *s)
{
  char *source = s;
  char c;
  char *target = (char *) g_malloc(1 + strlen(s) * 2); /* We will probably need less than that */
  char *writer = target;

  while (c = *source++) {

    if (c < 32) { /* Special character? */
      *writer++ = '\\'; /* Escape... */
      c += ('a' - 1);
    } else if (c == '\\' || c == '"')
      *writer++ = '\\'; /* Escape, but do not change */
    *writer++ = c;

  }
  *writer = 0; /* Terminate string */

  return (char *) g_realloc(target, strlen(target) + 1);
}


static char *
_cfsml_unmangle_string(char *s)
{
  char *target = (char *) g_malloc(1 + strlen(s));
  char *writer = target;
  char *source = s;
  char c;

  while ((c = *source++) && (c > 31)) {
    if (c == '\\') { /* Escaped character? */
      c = *source++;
      if ((c != '\\') && (c != '"')) /* Un-escape 0-31 only */
	c -= ('a' - 1);
    }
    *writer++ = c;
  }
  *writer = 0; /* Terminate string */

  return (char *) g_realloc(target, strlen(target) + 1);
}


static char *
_cfsml_get_identifier(FILE *fd, int *line, int *hiteof, int *assignment)
{
  char c;
  int mem = 32;
  int pos = 0;
  int done = 0;
  char *retval = (char *) g_malloc(mem);

  while (isspace(c = fgetc(fd)) && (c != EOF));
  if (c == EOF) {
    _cfsml_error("Unexpected end of file at line %d\n", *line);
    free(retval);
    *hiteof = 1;
    return NULL;
  }

  ungetc(c, fd);

  while (((c = fgetc(fd)) != EOF) && ((pos == 0) || (c != '\n')) && (c != '=')) {

     if (pos == mem - 1) /* Need more memory? */
       retval = (char *) g_realloc(retval, mem *= 2);

     if (!isspace(c)) {
        if (done) {
           _cfsml_error("Single word identifier expected at line %d\n", *line);
           free(retval);
           return NULL;
        }
        retval[pos++] = c;
     } else
        if (pos != 0)
           done = 1; /* Finished the variable name */
        else if (c == '\n')
           ++(*line);
  }

  if (c == EOF) {
    _cfsml_error("Unexpected end of file at line %d\n", *line);
    free(retval);
    *hiteof = 1;
    return NULL;
  }

  if (c == '\n') {
    ++(*line);
    if (assignment)
      *assignment = 0;
  } else
    if (assignment)
      *assignment = 1;

  if (pos == 0) {
    _cfsml_error("Missing identifier in assignment at line %d\n", *line);
    free(retval);
    return NULL;
  }

  if (pos == mem - 1) /* Need more memory? */
     retval = (char *) g_realloc(retval, mem += 1);

  retval[pos] = 0; /* Terminate string */

  return retval;
}


static char *
_cfsml_get_value(FILE *fd, int *line, int *hiteof)
{
  char c;
  int mem = 64;
  int pos = 0;
  char *retval = (char *) g_malloc(mem);

  while (((c = fgetc(fd)) != EOF) && (c != '\n')) {

     if (pos == mem - 1) /* Need more memory? */
       retval = (char *) g_realloc(retval, mem *= 2);

     if (pos || (!isspace(c)))
        retval[pos++] = c;

  }

  while ((pos > 0) && (isspace(retval[pos - 1])))
     --pos; /* Strip trailing whitespace */

  if (c == EOF)
    *hiteof = 1;

  if (pos == 0) {
    _cfsml_error("Missing value in assignment at line %d\n", *line);
    free(retval);
    return NULL;
  }

  if (c == '\n')
     ++(*line);

  if (pos == mem - 1) /* Need more memory? */
    retval = (char *) g_realloc(retval, mem += 1);

  retval[pos] = 0; /* Terminate string */
  return (char *) g_realloc(retval, strlen(retval));
  /* Re-allocate; this value might be used for quite some while (if we are
  ** restoring a string)
  */
}
#line 284 "cfsml.pl"
static void
_cfsml_write_sound_lib_file_t(FILE *fh, sound_lib_file_t* foo);
static int
_cfsml_read_sound_lib_file_t(FILE *fh, sound_lib_file_t* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_song_t(FILE *fh, song_t* foo);
static int
_cfsml_read_song_t(FILE *fh, song_t* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_int(FILE *fh, int* foo);
static int
_cfsml_read_int(FILE *fh, int* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_word(FILE *fh, word* foo);
static int
_cfsml_read_word(FILE *fh, word* foo, char *lastval, int *line, int *hiteof);

#line 284 "cfsml.pl"
static void
_cfsml_write_string(FILE *fh, char ** foo);
static int
_cfsml_read_string(FILE *fh, char ** foo, char *lastval, int *line, int *hiteof);

#line 297 "cfsml.pl"
static void
_cfsml_write_sound_lib_file_t(FILE *fh, sound_lib_file_t* foo)
{
  char *bar;
  int min, max, i;

#line 315 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "songs = ");
    min = max = foo->songs_nr;
    if (!foo->songs)
       min = max = 0; /* Don't write if it points to NULL */
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_song_t(fh, &(foo->songs[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "active_song = ");
    _cfsml_write_int(fh, &(foo->active_song));
    fprintf(fh, "\n");
  fprintf(fh, "soundcue = ");
    _cfsml_write_int(fh, &(foo->soundcue));
    fprintf(fh, "\n");
  fprintf(fh, "usecs_to_sleep = ");
    _cfsml_write_int(fh, &(foo->usecs_to_sleep));
    fprintf(fh, "\n");
  fprintf(fh, "ticks_to_wait = ");
    _cfsml_write_int(fh, &(foo->ticks_to_wait));
    fprintf(fh, "\n");
  fprintf(fh, "ticks_to_fade = ");
    _cfsml_write_int(fh, &(foo->ticks_to_fade));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 381 "cfsml.pl"
static int
_cfsml_read_sound_lib_file_t(FILE *fh, sound_lib_file_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 435 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d\n",*line);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    bar = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!bar)
       return CFSML_FAILURE;
    if (!assignment) {
      if (!strcmp(bar, "}")) 
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(bar, "songs")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
#line 497 "cfsml.pl"
         /* Prepare to restore dynamic array */
         max = strtol(value + 1, NULL, 0);
         if (max < 0) {
            _cfsml_error("Invalid number of elements to allocate for dynamic array '%s' at line %d\n", bar, *line);
            return CFSML_FAILURE;
;         }

         if (max)
           foo->songs = (song_t *) g_malloc(max * sizeof(song_t));
         else
           foo->songs = NULL;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_song_t(fh, &(foo->songs[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
         foo->songs_nr = max ; /* Set array size accordingly */
      } else
      if (!strcmp(bar, "active_song")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->active_song), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "soundcue")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->soundcue), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "usecs_to_sleep")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->usecs_to_sleep), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ticks_to_wait")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->ticks_to_wait), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "ticks_to_fade")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->ticks_to_fade), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 560 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }

    free (bar);
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_song_t(FILE *fh, song_t* foo)
{
  char *bar;
  int min, max, i;

#line 315 "cfsml.pl"
  fprintf(fh, "{\n");
  fprintf(fh, "flags = ");
    min = max = MIDI_CHANNELS;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(foo->flags[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "instruments = ");
    min = max = MIDI_CHANNELS;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(foo->instruments[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "velocity = ");
    min = max = MIDI_CHANNELS;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(foo->velocity[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "pressure = ");
    min = max = MIDI_CHANNELS;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(foo->pressure[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "pitch = ");
    min = max = MIDI_CHANNELS;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(foo->pitch[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "channel_map = ");
    min = max = MIDI_CHANNELS;
#line 341 "cfsml.pl"
    fprintf(fh, "[%d][\n", max);
    for (i = 0; i < min; i++) {
      _cfsml_write_int(fh, &(foo->channel_map[i]));
      fprintf(fh, "\n");
    }
    fprintf(fh, "]");
    fprintf(fh, "\n");
  fprintf(fh, "size = ");
    _cfsml_write_int(fh, &(foo->size));
    fprintf(fh, "\n");
  fprintf(fh, "pos = ");
    _cfsml_write_int(fh, &(foo->pos));
    fprintf(fh, "\n");
  fprintf(fh, "loopmark = ");
    _cfsml_write_int(fh, &(foo->loopmark));
    fprintf(fh, "\n");
  fprintf(fh, "fading = ");
    _cfsml_write_int(fh, &(foo->fading));
    fprintf(fh, "\n");
  fprintf(fh, "file_nr = ");
    _cfsml_write_int(fh, &(foo->file_nr));
    fprintf(fh, "\n");
  fprintf(fh, "priority = ");
    _cfsml_write_int(fh, &(foo->priority));
    fprintf(fh, "\n");
  fprintf(fh, "loops = ");
    _cfsml_write_int(fh, &(foo->loops));
    fprintf(fh, "\n");
  fprintf(fh, "status = ");
    _cfsml_write_int(fh, &(foo->status));
    fprintf(fh, "\n");
  fprintf(fh, "handle = ");
    _cfsml_write_word(fh, &(foo->handle));
    fprintf(fh, "\n");
  fprintf(fh, "}");
}

#line 381 "cfsml.pl"
static int
_cfsml_read_song_t(FILE *fh, song_t* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 435 "cfsml.pl"
  int assignment, closed, done;

  if (strcmp(lastval, "{")) {
     _cfsml_error("Reading record; expected opening braces in line %d\n",*line);
     return CFSML_FAILURE;
  };
  closed = 0;
  do {
    char *value;
    bar = _cfsml_get_identifier(fh, line, hiteof, &assignment);

    if (!bar)
       return CFSML_FAILURE;
    if (!assignment) {
      if (!strcmp(bar, "}")) 
         closed = 1;
      else {
        _cfsml_error("Expected assignment or closing braces in line %d\n", *line);
        return CFSML_FAILURE;
      }
    } else {
      value = "";
      while (!value || !strcmp(value, ""))
        value = _cfsml_get_value(fh, line, hiteof);
      if (!value)
         return CFSML_FAILURE;
      if (!strcmp(bar, "flags")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(foo->flags[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "instruments")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(foo->instruments[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "velocity")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(foo->velocity[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "pressure")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(foo->pressure[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "pitch")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(foo->pitch[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "channel_map")) {
#line 487 "cfsml.pl"
         if ((value[0] != '[') || (value[strlen(value) - 1] != '[')) {
            _cfsml_error("Opening brackets expected at line %d\n", *line);
            return CFSML_FAILURE;
;         }
         /* Prepare to restore static array */
         max = MIDI_CHANNELS;
#line 518 "cfsml.pl"
         done = i = 0;
         do {
           free(value);
           if (!(value = _cfsml_get_identifier(fh, line, hiteof, NULL)))
#line 527 "cfsml.pl"
              return 1;
           if (strcmp(value, "]")) {
             if (i == max) {
               _cfsml_error("More elements than space available (%d) in '%s' at line %d\n", max, bar, *line);
               return CFSML_FAILURE;
             }
             if (_cfsml_read_int(fh, &(foo->channel_map[i++]), value, line, hiteof))
                return CFSML_FAILURE;
           } else done = 1;
         } while (!done);
      } else
      if (!strcmp(bar, "size")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->size), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "pos")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->pos), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "loopmark")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->loopmark), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "fading")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->fading), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "file_nr")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->file_nr), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "priority")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->priority), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "loops")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->loops), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "status")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_int(fh, &(foo->status), value, line, hiteof))
            return CFSML_FAILURE;
      } else
      if (!strcmp(bar, "handle")) {
#line 553 "cfsml.pl"
         if (_cfsml_read_word(fh, &(foo->handle), value, line, hiteof))
            return CFSML_FAILURE;
      } else
#line 560 "cfsml.pl"
       {
          _cfsml_error("Assignment to invalid identifier '%s' in line %d\n", bar, *line);
          return CFSML_FAILURE;       }
     }

    free (bar);
  } while (!closed); /* Until closing braces are hit */
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_int(FILE *fh, int* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 381 "cfsml.pl"
static int
_cfsml_read_int(FILE *fh, int* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 405 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_word(FILE *fh, word* foo)
{
  char *bar;
  int min, max, i;

  fprintf(fh, "%li", (long) *foo);
}

#line 381 "cfsml.pl"
static int
_cfsml_read_word(FILE *fh, word* foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 405 "cfsml.pl"

  *foo = strtol(lastval, &bar, 0);
  if (*bar != 0) {
     _cfsml_error("Non-integer encountered while parsing int value at line %d\n", *line);
     return CFSML_FAILURE;
  }
  return CFSML_SUCCESS;
}

#line 297 "cfsml.pl"
static void
_cfsml_write_string(FILE *fh, char ** foo)
{
  char *bar;
  int min, max, i;

#line 305 "cfsml.pl"
  if (!(*foo))
    fprintf(fh, "\\null\\");  else {
    bar = _cfsml_mangle_string((char *) *foo);
    fprintf(fh, "\"%s\"", bar);
    free(bar);
  }
}

#line 381 "cfsml.pl"
static int
_cfsml_read_string(FILE *fh, char ** foo, char *lastval, int *line, int *hiteof)
{
  char *bar;
  int min, max, i;
#line 414 "cfsml.pl"

  if (strcmp(lastval, "\\null\\")) { /* null pointer? */
    if (*lastval == '"') { /* Quoted string? */
      int seeker = strlen(lastval);

      while (lastval[seeker] != '"')
        --seeker;

      if (!seeker) { /* No matching double-quotes? */
        _cfsml_error("Unbalanced quotes at line %d\n", *line);
        return CFSML_FAILURE;
      }

      lastval[seeker] = 0; /* Terminate string at closing quotes... */
      lastval++; /* ...and skip the opening quotes locally */
    }
    *foo = _cfsml_unmangle_string(lastval);
    return CFSML_SUCCESS;
  } else {
    *foo = NULL;
    return CFSML_SUCCESS;
  }
}


/* Auto-generated CFSML declaration and function block ends here */
/* Auto-generation performed by cfsml.pl 0.6.6 */
#line 96 "CFSML input file"

/* Sound state saving reference implementation */
int
soundsrv_save_state(FILE *debugstream, char *dir, songlib_t songlib, song_t *curr_song,
		    int soundcue, int usecs_to_sleep, int ticks_to_wait, int ticks_to_fade)
{
  sound_lib_file_t write_rec;
  song_t *seeker;
  int songctr = 0;
  int curr_song_nr = -1;
  FILE *fh;

  if (chdir(dir)) {
    char *cwd;

    cwd = getcwd(NULL, 0);
    fprintf(debugstream, "Failed to enter '%s', cwd is '%s'\n", dir, cwd);
    free(cwd);
    return 1;
  }

  write_rec.soundcue = soundcue;
  write_rec.usecs_to_sleep = usecs_to_sleep;
  write_rec.ticks_to_wait = ticks_to_wait;
  write_rec.ticks_to_fade = ticks_to_fade;

  /* Determine number of songs */
  seeker = *songlib;
  while (seeker) {

    if (seeker == curr_song)
      curr_song_nr = songctr;

    ++songctr;
    seeker = seeker->next;
  }

  write_rec.songs_nr = songctr;
  write_rec.active_song = curr_song_nr;

  write_rec.songs = malloc(sizeof(song_t) * songctr);

  /* Now memcpy those songs into a row and write their song data to a file */
  songctr = -1;
  seeker = *songlib;
  while (seeker) {
    char filename[10];
    int fd;
    ++songctr;

    memcpy(&(write_rec.songs[songctr]), seeker, sizeof(song_t));
    write_rec.songs[songctr].file_nr = songctr;

    /* Now write to an external file */
    sprintf(filename, "song.%d", songctr);
    fd = creat(filename, 0600);

    if (fd < 0) {
      fprintf(debugstream, "Error creating file: %s while creating '%s/%s'\n",
	      strerror(errno), dir, filename);
      free(write_rec.songs);
      chdir ("..");
      return 1;
    }

    if (write(fd, seeker->data, seeker->size) < seeker->size) {
      fprintf(debugstream, "Write error: Failed to write to '%s/%s'\n", dir, filename);
      free(write_rec.songs);
      chdir ("..");
      return 1;
    }
    close(fd);

    seeker = seeker->next;
  }

  fh = fopen("sound", "w");

#line 661 "cfsml.pl"
/* Auto-generated CFSML data writer code */
  _cfsml_write_sound_lib_file_t(fh, &write_rec);
  fprintf(fh, "\n");
/* End of auto-generated CFSML data writer code */
#line 176 "CFSML input file"

  fclose(fh);
  fprintf(stderr,"Finished all writing.\n");

  free(write_rec.songs);

  chdir ("..");
  return 0;
}

/* Sound state restore complement for the saving reference implementation */
int
soundsrv_restore_state(FILE *debugstream, char *dir, songlib_t songlib, song_t **curr_song,
		       int *soundcue, int *usecs_to_sleep, int *ticks_to_wait, int *ticks_to_fade)
{
  FILE *fh;
  sound_lib_file_t read_rec;
  song_t *seeker, *next = NULL;
  int error;
  int i;

  if (chdir(dir)) {
    fprintf(debugstream, "Failed to enter '%s'\n", dir);
    return 1;
  }

  fh = fopen("sound", "r");

  if (!fh) {
    fprintf(debugstream, "'%s/sound' not found!\n", dir);
    chdir ("..");
    return 1;
  }

  read_rec.songs = NULL;

/* Auto-generated CFSML data reader code */
#line 626 "cfsml.pl"
  {
    int _cfsml_line_ctr = 0;
    int _cfsml_eof = 0, _cfsml_error;
    int dummy;
    char *_cfsml_inp = _cfsml_get_identifier(fh, &(_cfsml_line_ctr), &_cfsml_eof, &dummy);

    _cfsml_error = _cfsml_read_sound_lib_file_t(fh, &read_rec, _cfsml_inp, &(_cfsml_line_ctr), &_cfsml_eof);
    free(_cfsml_inp);
    error = _cfsml_error;
  }
/* End of auto-generated CFSML data reader code */
#line 213 "CFSML input file"

  if (error) {
    if(read_rec.songs)
      free(read_rec.songs);

    chdir ("..");
    fprintf(debugstream, "Failed to parse '%s/sound'\n", dir);
    return 1;
  }

  for (i = 0; i < read_rec.songs_nr; i++) {
    int size = read_rec.songs[i].size;
    char filename[10];
    int fd;

    sprintf(filename, "song.%d", read_rec.songs[i].file_nr);
    fd = open(filename, O_BINARY | O_RDONLY);

    if (fd == -1) {
      chdir ("..");
      fprintf(debugstream, "Opening %s/%s failed: %s\n", dir, filename, strerror(errno));
      if(read_rec.songs)
	free(read_rec.songs);
      return 1;
    }

    if (read(fd, read_rec.songs[i].data = (byte *) malloc(size), size) < size) {
      int j;
      for (j = 0; i < i; j++)
	free(read_rec.songs[i].data);
      chdir ("..");
      fprintf(debugstream, "Reading %d bytes from %s/%s failed\n", size, dir, filename);
      return 1;
    }

    close(fd);
  }


  song_lib_free(songlib); /* Everything is well, so free all  songs */

  *curr_song = NULL;

  for (i = 0; i < read_rec.songs_nr; i++) {

    next = g_memdup(&(read_rec.songs[i]), sizeof(song_t));
    next->next = NULL;
    if (i > 0)
      seeker->next = next;

    seeker = next;

    if (i == 0)
      *songlib = seeker;

    if (i == read_rec.active_song)
      *curr_song = seeker;
  }

  *soundcue = read_rec.soundcue;
  *usecs_to_sleep = read_rec.usecs_to_sleep;
  *ticks_to_wait = read_rec.ticks_to_wait;
  *ticks_to_fade = read_rec.ticks_to_fade;

  if (read_rec.songs_nr)
    free(read_rec.songs);

  chdir ("..");

  return 0;
}