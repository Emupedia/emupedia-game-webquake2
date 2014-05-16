#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdio.h>

#include "../client/client.h"
#include "../client/snd_loc.h"

int audio_fd;
int snd_inited = 0;

cvar_t *sndbits;
cvar_t *sndspeed;
cvar_t *sndchannels;
cvar_t *snddevice;

static int tryrates[] = { 11025, 22051, 44100, 8000 };


int SNDDMA_GetDMAPos(void)
{
	if (!snd_inited) return 0;

	return 0;

}

void SNDDMA_Shutdown(void)
{
#if 0
	if (snd_inited)
	{
		close(audio_fd);
		snd_inited = 0;
	}
#endif
}

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
}

void SNDDMA_BeginPainting (void)
{
}

