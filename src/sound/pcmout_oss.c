/***************************************************************************
 pcmout_sdl.c Copyright (C) 2002 Solomon Peachy (And Claudio Matsuoka)

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

#include <pcmout.h>

#if defined(HAVE_SYS_SOUNDCARD_H) && defined(HAVE_PTHREADS)
#include <sys/soundcard.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pthread.h>

static pthread_t thread;
static int run = 1;

static gint16 *buffer;

static char *oss_device = "/dev/audio"; 

static int oss_fd;

static void *sound_thread (void *arg)
{
	int count, j;
	gint16 *b = buffer;
	int shift = (pcmout_stereo) ? 2: 1;

	while(run) {
		count = mix_sound(BUFFER_SIZE) << shift;  
		b = buffer;

		do {
			if ((j = write (oss_fd, b, count)) > 0) {
				count -= j;
				b += j;
			}
		} while (count);
	}
	pthread_exit(0);
}

static int pcmout_oss_open(gint16 *b, guint16 rate, guint8 stereo) 
{
	audio_buf_info info;
	int i;

	buffer = b;
  
	if ((oss_fd = open (oss_device, O_WRONLY)) == -1) {
		fprintf(stderr, "Can't open %s\n", oss_device);
		return -1;
	}

	if (ioctl (oss_fd, SNDCTL_DSP_RESET)) {
		fprintf(stderr, "[PCM-OSS] Failed to reset device\n");
		return -1;
	}

	i = AFMT_S16_NE;  /* Use NATIVE endian format... */
	if (ioctl (oss_fd, SNDCTL_DSP_SETFMT, &i)) {
		fprintf(stderr, "[PCM-OSS] Failed to set device output format\n");
		return -1;
	}

	i = (4 << 16 | 13);  /* ask for 4 fragments of 2^13 bytes each */
	if (ioctl (oss_fd, SNDCTL_DSP_SETFRAGMENT, &i)) {
		fprintf(stderr, "[PCM-OSS] Failed to set fragment size as requested\n");
		return -1;
	}

	if (ioctl (oss_fd, SNDCTL_DSP_GETOSPACE, &info)) {
		fprintf(stderr, "[PCM-OSS] Failed to a get fragment size\n");
		return -1;
	}

	printf ("Using %d fragments of %d bytes.\n", 
		info.fragstotal, info.fragsize);
	  
	i = rate;
	if (ioctl (oss_fd, SNDCTL_DSP_SPEED, &i)) {
		fprintf(stderr, "[PCM-OSS] Failed to set DSP output speed\n");
		return -1;
	}

       	i = stereo;
	if (ioctl (oss_fd, SNDCTL_DSP_STEREO, &i)) {
		fprintf(stderr, "[PCM-OSS] Failed to set stereo to %d\n", stereo);
		if (stereo)
			return -1;
		fprintf(stderr, "[PCM-OSS] Not fatal, continuing...\n");
	}

	pthread_create (&thread, NULL, sound_thread, NULL);
	return 0;
}

static int pcmout_oss_close() {
  int err;
  
  run = 0;

  pthread_join(thread, NULL);

  ioctl (oss_fd, SNDCTL_DSP_SYNC);
  close (oss_fd);

  return 0;
}

pcmout_driver_t pcmout_driver_oss = {
  "oss",
  "v0.01",
  NULL,
  &pcmout_oss_open,
  &pcmout_oss_close
};

#endif  /* HAVE_SOUNDCARD_H */
