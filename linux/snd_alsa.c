/*
	snd_alsa.c

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		59 Temple Place - Suite 330
		Boston, MA  02111-1307, USA

	$Id: snd_alsa.c,v 1.5 2005/01/02 03:29:11 bburns Exp $
*/

//    quake2-r0.16.1/src/linux/snd_alsa.c 2010-08-29 09:58:50
//    Alsa Redux Patch
#define BUFFER_SIZE 16384

#include <alsa/asoundlib.h>

#include "../client/client.h"
#include "../client/snd_loc.h"

#define AFSize 4 
static int  snd_inited=0;
static snd_pcm_t *handle;
static int AlsaFrames;
static int AlsaPos;

/*
#define snd_buf BUFFER_SIZE

static int  snd_inited;
static short *buffer;

static snd_pcm_t *playback_handle;
static snd_pcm_hw_params_t *hw_params;

cvar_t *sndbits;
cvar_t *sndspeed;
cvar_t *sndchannels;
cvar_t *snddevice;

static int tryrates[] = { 44100, 22051, 11025, 8000 };
*/
qboolean SNDDMA_Init (void)
{
  int confirm;
  int latency;
  Com_Printf("Setting up ALSA driver .....\n");

  if (snd_inited) { return 1; }
  int s_khz=(Cvar_Get("s_khz", "22", CVAR_ARCHIVE))->value;
  if (s_khz <22 ){
    Com_Printf("Low Performance 11KHz.\n");
    Cvar_Set("s_mixahead","0.2");
    dma.speed=11025;
    AlsaFrames=2048;
    latency=200000;
    }else{
    Com_Printf("High Quality 48KHz.\n");     
        Cvar_Set("s_mixahead","0.1");      
     dma.speed=48000;
    AlsaFrames=4800;
    latency=110000;}

  dma.channels = 2;
  dma.samplebits = 16;
  AlsaPos = 0;
  handle = NULL;
      confirm =
snd_pcm_open(&handle,"default",SND_PCM_STREAM_PLAYBACK,SND_PCM_NONBLOCK);
      if (confirm < 0) {
            Com_Printf("ALSA sound error: cannot open device \n"); 
            return 0;}
        confirm = snd_pcm_set_params(handle,
                    SND_PCM_FORMAT_S16_LE,
                                      SND_PCM_ACCESS_RW_INTERLEAVED,
                                      dma.channels,
                                      dma.speed,
                                      1,//soft resample
                                      latency);
     if (confirm < 0){ 
                Com_Printf("Alsa error: %s\n", snd_strerror(confirm));
                return 0;}
    confirm = snd_pcm_prepare(handle);
        if (confirm  < 0) {
        Com_Printf("...failed.\n");
         return 0;}

  int rambuffer = (BUFFER_SIZE+AlsaFrames)*AFSize;
  dma.buffer=malloc(rambuffer);    
  dma.samplepos = 0;
  dma.samples = BUFFER_SIZE*dma.channels;
  dma.submission_chunk = 16;
  /*
  sndbits = Cvar_Get("sndbits", "16", CVAR_ARCHIVE);
  sndspeed = Cvar_Get("sndspeed", "0", CVAR_ARCHIVE);
  sndchannels = Cvar_Get("sndchannels", "2", CVAR_ARCHIVE);
  snddevice = Cvar_Get("snddevice", "default", CVAR_ARCHIVE);
  
  err = snd_pcm_open(&playback_handle, snddevice->string, 
		     SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) {
    Com_Printf("ALSA snd error, cannot open device %s (%s)\n", 
	       snddevice->string,
	       snd_strerror(err));
    return 0;
  }
  
  err = snd_pcm_hw_params_malloc(&hw_params);

  if (err < 0) {
    Com_Printf("ALSA snd error, cannot allocate hw params (%s)\n",
	       snd_strerror(err));
    return 0;
  }
  
  err = snd_pcm_hw_params_any (playback_handle, hw_params);
  if (err < 0) {
    Com_Printf("ALSA snd error, cannot init hw params (%s)\n",
	       snd_strerror(err));
    snd_pcm_hw_params_free(hw_params);
    return 0;
  }
  
  err = snd_pcm_hw_params_set_access(playback_handle, hw_params, 
				     SND_PCM_ACCESS_RW_INTERLEAVED);
  if (err < 0) {
    Com_Printf("ALSA snd error, cannot set access (%s)\n",
	       snd_strerror(err));
    snd_pcm_hw_params_free(hw_params);
    return 0;
  }
  
  dma.samplebits = sndbits->value;
  if (dma.samplebits == 16 || dma.samplebits != 8) {
    err = snd_pcm_hw_params_set_format(playback_handle, hw_params,
				       SND_PCM_FORMAT_S16_LE);
    if (err < 0) {
      Com_Printf("ALSA snd error, 16 bit sound not supported, trying 8\n");
      dma.samplebits = 8;
    }
    else {
      format = SND_PCM_FORMAT_S16_LE;
    }
  }
  if (dma.samplebits == 8) {
    err = snd_pcm_hw_params_set_format(playback_handle, hw_params,
				       SND_PCM_FORMAT_U8);
  }
  if (err < 0) {
    Com_Printf("ALSA snd error, cannot set sample format (%s)\n",
	       snd_strerror(err));
    snd_pcm_hw_params_free(hw_params);
    return 0;
  }
  else {
    format = SND_PCM_FORMAT_U8;
  }
  
  dma.speed = (int)sndspeed->value;
  if (!dma.speed) {
    for (i=0 ; i<sizeof(tryrates); i++) {
      int test = tryrates[i];
      err = snd_pcm_hw_params_set_rate_near(playback_handle, hw_params,
					    &test, 0);
      if (err < 0) {
	Com_Printf("ALSA snd error, cannot set sample rate %d (%s)\n",
		   tryrates[i], snd_strerror(err));
      }
      else {
	dma.speed = test;
	break;
      }
    }
  }
  if (!dma.speed) {
    Com_Printf("ALSA snd error couldn't set rate.\n");
    snd_pcm_hw_params_free(hw_params);
    return 0;
  }

  dma.channels = (int)sndchannels->value;
  if (dma.channels < 1 || dma.channels > 2)
    dma.channels = 2;
  err = snd_pcm_hw_params_set_channels(playback_handle,hw_params,dma.channels);
  if (err < 0) {
    Com_Printf("ALSA snd error couldn't set channels %d (%s).\n",
	       dma.channels, snd_strerror(err));
    snd_pcm_hw_params_free(hw_params);
    return 0;
  }
  
  err = snd_pcm_hw_params(playback_handle, hw_params);
  if (err < 0) {
    Com_Printf("ALSA snd error couldn't set params (%s).\n",snd_strerror(err));
    snd_pcm_hw_params_free(hw_params);
    return 0;
  }

    buffer_size = snd_pcm_hw_params_get_buffer_size(hw_params);
    frame_size = (snd_pcm_format_physical_width(format)*dma.channels)/8;
    
    snd_pcm_hw_params_free(hw_params);
    hw_params = NULL;
    
    if ((err = snd_pcm_prepare(playback_handle)) < 0) {
    Com_Printf("ALSA snd error preparing audio (%s)\n",snd_strerror(err));
    return 0;
    }
    
    snd_buf = buffer_size*frame_size;

  //snd_buf = BUFFER_SIZE;

  buffer=malloc(snd_buf);
  memset(buffer, 0, snd_buf);

  dma.samplepos = 0;
  dma.samples = snd_buf / (dma.samplebits/8);
  dma.submission_chunk = 1;
  dma.buffer = (char *)buffer;
  */
  snd_inited = 1;
  return 1;
}

int
SNDDMA_GetDMAPos (void)
{
    int timerpos;
    int send;
    int taken;
    timerpos = snd_pcm_avail(handle);
    if (timerpos<0){// likely buffer underrun.
        timerpos = snd_pcm_prepare(handle);
        timerpos=0;}

    send=AlsaFrames-timerpos;
    if ( (send+AlsaPos)>BUFFER_SIZE )
    {    memcpy (dma.buffer+BUFFER_SIZE*AFSize,
                dma.buffer,AlsaFrames*AFSize);}
    taken = snd_pcm_writei(handle, dma.buffer+AlsaPos*AFSize, send);    
    if (taken<0) {taken=0;}    

    AlsaPos+=taken;
    if (BUFFER_SIZE<=AlsaPos){AlsaPos -= BUFFER_SIZE;}
    return AlsaPos*dma.channels; 

}

void
SNDDMA_Shutdown (void)
{
  if (snd_inited) {
    snd_pcm_drop(handle);
    snd_pcm_close(handle);
    snd_inited = 0;
    free(dma.buffer);
  }
  dma.buffer = NULL;
}

/*
  SNDDMA_Submit
Send sound to device if buffer isn't really the dma buffer
*/
void
SNDDMA_Submit (void)
{
}


void SNDDMA_BeginPainting(void)
{    
}
