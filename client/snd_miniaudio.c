/*
Copyright (C) 2018-2019 Krzysztof Kondrak

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifdef _WIN32
#  include <windows.h>
#endif

#define DR_FLAC_IMPLEMENTATION
#include "miniaudio/dr_flac.h"  /* Enables FLAC decoding. */
#define DR_MP3_IMPLEMENTATION
#include "miniaudio/dr_mp3.h"   /* Enables MP3 decoding. */
#define DR_WAV_IMPLEMENTATION
#include "miniaudio/dr_wav.h"   /* Enables WAV decoding. */

#include "miniaudio/stb_vorbis.c" /* Enables OGG/Vorbis decoding. */

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "../client/client.h"

static ma_decoder decoder;
static ma_device device;
static int loopcounter;
static int playTrack = 0;
static qboolean enabled = true;
static qboolean paused = false;
static qboolean playLooping = false;
static qboolean trackFinished = false;

static cvar_t *cd_nocd;
static cvar_t *cd_loopcount;
static cvar_t *cd_looptrack;

static void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
	if (pDecoder == NULL) {
		return;
	}

	// playback completed
	if (!ma_decoder_read_pcm_frames(pDecoder, pOutput, frameCount))
	{
		trackFinished = true;
	}

	(void)pInput;
}

static void Miniaudio_Pause(void)
{
	if (!enabled || !ma_device_is_started(&device) || paused)
		return;

	ma_device_stop(&device);
	paused = true;
}

static void Miniaudio_Resume(void)
{
	if (!enabled || !paused)
		return;

	ma_device_start(&device);
	paused = false;
}

static void Miniaudio_f(void)
{
	char	*command;

	if (Cmd_Argc() < 2)
		return;

	command = Cmd_Argv(1);

	if (Q_strcasecmp(command, "on") == 0)
	{
		enabled = true;
		return;
	}

	if (Q_strcasecmp(command, "off") == 0)
	{
		Miniaudio_Stop();
		enabled = false;
		return;
	}

	if (Q_strcasecmp(command, "play") == 0)
	{
		Miniaudio_Play(atoi(Cmd_Argv(2)), false);
		return;
	}

	if (Q_strcasecmp(command, "loop") == 0)
	{
		Miniaudio_Play(atoi(Cmd_Argv(2)), true);
		return;
	}

	if (Q_strcasecmp(command, "stop") == 0)
	{
		Miniaudio_Stop();
		return;
	}

	if (Q_strcasecmp(command, "pause") == 0)
	{
		Miniaudio_Pause();
		return;
	}

	if (Q_strcasecmp(command, "resume") == 0)
	{
		Miniaudio_Resume();
		return;
	}

	if (Q_strcasecmp(command, "info") == 0)
	{
		if (ma_device_is_started(&device))
			Com_Printf("Currently %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		else if (paused)
			Com_Printf("Paused %s track %u\n", playLooping ? "looping" : "playing", playTrack);
		else
			Com_Printf("No music is playing.\n");
		return;
	}
}


void Miniaudio_Init(void)
{
	cd_nocd = Cvar_Get("cd_nocd", "0", CVAR_ARCHIVE);
	cd_loopcount = Cvar_Get("cd_loopcount", "4", 0);
	cd_looptrack = Cvar_Get("cd_looptrack", "11", 0);
	enabled = true;
	paused = false;

	Cmd_AddCommand("miniaudio", Miniaudio_f);
}

void Miniaudio_Play(int track, qboolean looping)
{
	ma_result result;
	ma_device_config deviceConfig;
	int trackExtIdx = 0;
	static char *trackExts[] = { "ogg", "flac", "mp3", "wav" };

	if (!enabled || playTrack == track)
		return;

	Miniaudio_Stop();

	do
	{
		char trackPath[64];
		Com_sprintf(trackPath, sizeof(trackPath), "%s/music/track%s%i.%s", FS_Gamedir(), track < 10 ? "0" : "", track, trackExts[trackExtIdx]);
		result = ma_decoder_init_file(trackPath, NULL, &decoder);
	} while (result != MA_SUCCESS && ++trackExtIdx < 4);

	if (result != MA_SUCCESS)
	{
		Com_Printf("Failed to open %s/music/track%s%i.[ogg/flac/mp3/wav]: error %i\n", FS_Gamedir(), track < 10 ? "0" : "", track, result);
		return;
	}

	deviceConfig = ma_device_config_init(ma_device_type_playback);
	deviceConfig.playback.format = decoder.outputFormat;
	deviceConfig.playback.channels = decoder.outputChannels;
	deviceConfig.sampleRate = decoder.outputSampleRate;
	deviceConfig.dataCallback = data_callback;
	deviceConfig.pUserData = &decoder;

	if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
		Com_Printf("Failed to open playback device: error %i\n", result);
		ma_decoder_uninit(&decoder);
		return;
	}

	if (ma_device_start(&device) != MA_SUCCESS) {
		Com_Printf("Failed to start playback device: error %i\n", result);
		ma_device_uninit(&device);
		ma_decoder_uninit(&decoder);
		return;
	}

	loopcounter = 0;
	playTrack = track;
	playLooping = looping;
	trackFinished = false;

	if (Cvar_VariableValue("cd_nocd"))
		Miniaudio_Pause();
}

void Miniaudio_Stop(void)
{
	if (!enabled || !ma_device_is_started(&device))
		return;

	paused = false;
	playTrack = 0;

	ma_device_uninit(&device);
	ma_decoder_uninit(&decoder);
}

void Miniaudio_Update(void)
{
	if (cd_nocd->value != !enabled)
	{
		if (cd_nocd->value)
		{
			Miniaudio_Pause();
			enabled = false;
		}
		else
		{
			enabled = true;
			Miniaudio_Resume();
		}
	}

	if (enabled && ma_device_is_started(&device) && trackFinished)
	{
		trackFinished = false;
		if (playLooping)
		{
			// if the track has played the given number of times,
			// go to the ambient track
			if (++loopcounter >= cd_loopcount->value)
			{
				// ambient track will be played for the first time, so we need to load it
				if (loopcounter == cd_loopcount->value)
				{
					Miniaudio_Stop();
					Miniaudio_Play(cd_looptrack->value, true);
				}
				else
				{
					ma_decoder_seek_to_pcm_frame(&decoder, 0);
				}
			}
			else
			{
				ma_decoder_seek_to_pcm_frame(&decoder, 0);
			}
		}
	}
}

void Miniaudio_Shutdown(void)
{
	Miniaudio_Stop();
	Cmd_RemoveCommand("miniaudio");
}
