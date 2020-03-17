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

// music playback using Miniaudio library (https://github.com/dr-soft/miniaudio)

#ifdef _WIN32
#  include <windows.h>
#endif
// force ALSA on Linux to avoid playback issues when using mixed backends
#ifdef __linux__
#  define MA_NO_PULSEAUDIO
#  define MA_NO_JACK
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

static cvar_t *cd_volume;
static cvar_t *cd_loopcount;
static cvar_t *cd_looptrack;

#ifdef __APPLE__
static ma_uint32 periodSizeInFrames;

static UInt32 GetBytesPerSampleFrame()
{
	AudioDeviceID outputDeviceID;
	AudioStreamBasicDescription outputStreamBasicDescription;
	AudioObjectPropertyAddress propertyAddress = {
		kAudioHardwarePropertyDefaultOutputDevice,
		kAudioObjectPropertyScopeOutput,
		kAudioObjectPropertyElementMaster
	};

	UInt32 propertySize = sizeof(outputDeviceID);
	OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &propertySize, &outputDeviceID);
	if (status != kAudioHardwareNoError) {
		Com_Printf("AudioHardwareGetProperty returned %d\n", status);
		return 8;
	}

	propertySize = sizeof(outputStreamBasicDescription);
	propertyAddress.mSelector = kAudioDevicePropertyStreamFormat;
	status = AudioObjectGetPropertyData(outputDeviceID, &propertyAddress, 0, NULL, &propertySize, &outputStreamBasicDescription);
	if (status != kAudioHardwareNoError) {
		Com_Printf("AudioDeviceGetProperty: returned %d when getting kAudioDevicePropertyStreamFormat\n", status);
		return 8;
	}

	return outputStreamBasicDescription.mBytesPerFrame;
}
#endif

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
		if (device.pContext)
			Com_Printf("Using %s backend. ", ma_get_backend_name(device.pContext->backend));
		else
			Com_Printf("No audio backend enabled. ");

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
	cd_volume = Cvar_Get("cd_volume", "1", CVAR_ARCHIVE);
	cd_loopcount = Cvar_Get("cd_loopcount", "4", 0);
	cd_looptrack = Cvar_Get("cd_looptrack", "11", 0);
	enabled = true;
	paused = false;

#ifdef __APPLE__
	periodSizeInFrames = Cvar_VariableValue("s_chunksize") * sizeof(float) / GetBytesPerSampleFrame();
#endif
	Cmd_AddCommand("miniaudio", Miniaudio_f);
}

static ma_result LoadTrack(const char *gamedir, int track)
{
	ma_result result;
	int trackExtIdx = 0;
	static char *trackExts[] = { "ogg", "flac", "mp3", "wav" };

	do
	{
		char trackPath[64];
		Com_sprintf(trackPath, sizeof(trackPath), "%s/music/track%s%i.%s", gamedir, track < 10 ? "0" : "", track, trackExts[trackExtIdx]);
		result = ma_decoder_init_file(trackPath, NULL, &decoder);
	} while (result != MA_SUCCESS && ++trackExtIdx < 4);

	return result;
}

void Miniaudio_Play(int track, qboolean looping)
{
	ma_result result;
	ma_device_config deviceConfig;

	if (!enabled || playTrack == track)
		return;

	Miniaudio_Stop();

	// ignore invalid tracks
	if (track < 1)
		return;

	result = LoadTrack(FS_Gamedir(), track);

	// try the baseq2 folder if loading the track from a custom gamedir failed
	if (result != MA_SUCCESS && Q_stricmp(FS_Gamedir(), "./"BASEDIRNAME) != 0)
	{
		result = LoadTrack(BASEDIRNAME, track);
	}

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
#ifdef __APPLE__
	deviceConfig.periodSizeInFrames = periodSizeInFrames;
	deviceConfig.periods = 1;
#endif

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
	paused = false;
	trackFinished = false;

	if ( Cvar_VariableValue("cd_volume") == 0 )
		Miniaudio_Pause();

	ma_device_set_master_volume(&device, cd_volume->value);
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
	if (cd_volume->modified)
	{
		cd_volume->modified = false;

		if (cd_volume->value < 0.f)
			Cvar_SetValue("cd_volume", 0.f);
		if (cd_volume->value > 1.f)
			Cvar_SetValue("cd_volume", 1.f);

		ma_device_set_master_volume(&device, cd_volume->value);
	}

	if ((cd_volume->value == 0) != !enabled)
	{
		if (cd_volume->value == 0)
		{
			Miniaudio_Pause();
			enabled = false;
		}
		else
		{
			enabled = true;
			int track = atoi(cl.configstrings[CS_CDTRACK]);
			if (!paused || playTrack != track)
			{
				if ((playTrack == 0 && !playLooping) || (playTrack > 0 && playTrack != track))
					Miniaudio_Play(track, true);
				else
					Miniaudio_Play(playTrack, playLooping);
			}
			else
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
