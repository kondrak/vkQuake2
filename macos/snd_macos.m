/*
 Copyright (C) 1997-2005 Id Software, Inc.
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

// snd_macos.m
// CoreAudio sound support using an updated Quake III Arena driver

#include "../client/client.h"
#include "../client/snd_loc.h"

#include <CoreServices/CoreServices.h>
#include <CoreAudio/AudioHardware.h>

#import <Foundation/NSData.h>
#import <Foundation/NSString.h>

static unsigned int  submissionChunk;
static unsigned int  maxMixedSamples;
static short       *s_mixedSamples;
static int          s_chunkCount;		// number of chunks submitted
static qboolean     s_isRunning;

static AudioDeviceID outputDeviceID;
static AudioDeviceIOProcID outputDeviceProcID;
static AudioStreamBasicDescription outputStreamBasicDescription;

/*
===============
audioDeviceIOProc
===============
*/

OSStatus audioDeviceIOProc(AudioDeviceID inDevice,
                           const AudioTimeStamp *inNow,
                           const AudioBufferList *inInputData,
                           const AudioTimeStamp *inInputTime,
                           AudioBufferList *outOutputData,
                           const AudioTimeStamp *inOutputTime,
                           void *inClientData)
{
    int           offset;
    short        *samples;
    unsigned int  sampleIndex;
    float        *outBuffer;
    float         scale, temp;

    offset = ( s_chunkCount * submissionChunk ) % maxMixedSamples;
    samples = s_mixedSamples + offset;

    assert(outOutputData->mNumberBuffers == 1);
    assert(outOutputData->mBuffers[0].mNumberChannels == 2);
    //assert(outOutputData->mBuffers[0].mDataByteSize == (dma.submission_chunk * sizeof(float)));

    outBuffer = (float *)outOutputData->mBuffers[0].mData;

    // If we have run out of samples, return silence
    if (s_chunkCount * submissionChunk > dma.channels * paintedtime) {
        memset(outBuffer, 0, sizeof(*outBuffer) * dma.submission_chunk);
    } else {
        scale = (1.0f / SHRT_MAX);
        if (outputStreamBasicDescription.mSampleRate == 44100  && dma.speed == 22050) {
            for (sampleIndex = 0; sampleIndex < dma.submission_chunk; sampleIndex+=2) {
                // Convert the samples from shorts to floats.  Scale the floats to be [-1..1].
                temp = samples[sampleIndex + 0] * scale;
                outBuffer[(sampleIndex<<1)+0] = temp;
                outBuffer[(sampleIndex<<1)+2] = temp;

                temp = samples[sampleIndex + 1] * scale;
                outBuffer[(sampleIndex<<1)+1] = temp;
                outBuffer[(sampleIndex<<1)+3] = temp;
            }
        } else if (outputStreamBasicDescription.mSampleRate == 44100  && dma.speed == 11025) {
            for (sampleIndex = 0; sampleIndex < dma.submission_chunk; sampleIndex+=4) {
                // Convert the samples from shorts to floats.  Scale the floats to be [-1..1].
                temp = samples[sampleIndex + 0] * scale;
                outBuffer[(sampleIndex<<1)+0] = temp;
                outBuffer[(sampleIndex<<1)+2] = temp;
                outBuffer[(sampleIndex<<1)+4] = temp;
                outBuffer[(sampleIndex<<1)+6] = temp;

                temp = samples[sampleIndex + 1] * scale;
                outBuffer[(sampleIndex<<1)+1] = temp;
                outBuffer[(sampleIndex<<1)+3] = temp;
                outBuffer[(sampleIndex<<1)+5] = temp;
                outBuffer[(sampleIndex<<1)+7] = temp;
            }
        } else {
            for (sampleIndex = 0; sampleIndex < dma.submission_chunk; sampleIndex++) {
                // Convert the samples from shorts to floats.  Scale the floats to be [-1..1].
                outBuffer[sampleIndex] = samples[sampleIndex] * scale;
            }
        }
    }
    
    s_chunkCount++; // this is the next buffer we will submit
    return 0;
}


/*
===============
S_MakeTestPattern
===============
*/
void S_MakeTestPattern( void ) {
    int i;
    float v;
    int sample;
    
    for ( i = 0 ; i < dma.samples / 2 ; i ++ ) {
        v = sin( M_PI * 2 * i / 64 );
        sample = v * 0x4000;
        ((short *)dma.buffer)[i*2] = sample;	
        ((short *)dma.buffer)[i*2+1] = sample;	
    }
}

/*
===============
SNDDMA_Init
===============
*/
qboolean SNDDMA_Init(void)
{
    cvar_t *bufferSize;
    cvar_t *chunkSize;
    OSStatus status;
    UInt32 propertySize, bufferByteCount;

    if (s_isRunning)
        return true;
        
    chunkSize = Cvar_Get( "s_chunksize", "2048", CVAR_ARCHIVE );
    bufferSize = Cvar_Get( "s_buffersize", "16384", CVAR_ARCHIVE );
    Com_Printf(" Chunk size = %d\n", (int)chunkSize->value);
    Com_Printf("Buffer size = %d\n", (int)bufferSize->value);

    if (!chunkSize->value)
        Com_Error(ERR_FATAL, "s_chunksize must be non-zero\n");
    if (!bufferSize->value)
        Com_Error(ERR_FATAL, "s_buffersize must be non-zero\n");
    if (chunkSize->value >= bufferSize->value)
        Com_Error(ERR_FATAL, "s_chunksize must be less than s_buffersize\n");
    if ((int)bufferSize->value % (int)chunkSize->value)
        Com_Error(ERR_FATAL, "s_buffersize must be an even multiple of s_chunksize\n");

    // Get the output device
    propertySize = sizeof(outputDeviceID);
	AudioObjectPropertyAddress propertyAddress = {
		kAudioHardwarePropertyDefaultOutputDevice,
		kAudioObjectPropertyScopeOutput,
		kAudioObjectPropertyElementMaster
	};
	status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0, NULL, &propertySize, &outputDeviceID);
    if (status != kAudioHardwareNoError) {
        Com_Printf("AudioHardwareGetProperty returned %d\n", status);
        return false;
    }
	
    if (outputDeviceID == kAudioDeviceUnknown) {
        Com_Printf("AudioHardwareGetProperty: outputDeviceID is kAudioDeviceUnknown\n");
        return false;
    }

    // Configure the output device	
    propertySize = sizeof(bufferByteCount);
    bufferByteCount = chunkSize->value * sizeof(float);
	propertyAddress.mSelector = kAudioDevicePropertyBufferSize;
	status = AudioObjectSetPropertyData(outputDeviceID, &propertyAddress, 0, NULL, propertySize, &bufferByteCount);
    if (status != kAudioHardwareNoError) {
        Com_Printf("AudioDeviceSetProperty: returned %d when setting kAudioDevicePropertyBufferSize to %d\n", status, (int)chunkSize->value);
        return false;
    }
    
    propertySize = sizeof(bufferByteCount);
	status = AudioObjectGetPropertyData(outputDeviceID, &propertyAddress, 0, NULL, &propertySize, &bufferByteCount);
    if (status != kAudioHardwareNoError) {
        Com_Printf("AudioDeviceGetProperty: returned %d when getting kAudioDevicePropertyBufferSize\n", status);
        return false;
    }

    // Print out the device status
    propertySize = sizeof(outputStreamBasicDescription);
	propertyAddress.mSelector = kAudioDevicePropertyStreamFormat;
	status = AudioObjectGetPropertyData(outputDeviceID, &propertyAddress, 0, NULL, &propertySize, &outputStreamBasicDescription);
    if (status != kAudioHardwareNoError) {
        Com_Printf("AudioDeviceGetProperty: returned %d when getting kAudioDevicePropertyStreamFormat\n", status);
        return false;
    }

    Com_Printf("Hardware format:\n");
    Com_Printf("  %d mSampleRate\n", (int)outputStreamBasicDescription.mSampleRate);
    Com_Printf("  %c%c%c%c mFormatID\n",
               (outputStreamBasicDescription.mFormatID & 0xff000000) >> 24,
               (outputStreamBasicDescription.mFormatID & 0x00ff0000) >> 16,
               (outputStreamBasicDescription.mFormatID & 0x0000ff00) >>  8,
               (outputStreamBasicDescription.mFormatID & 0x000000ff) >>  0);
    Com_Printf("  %5d mBytesPerPacket\n", outputStreamBasicDescription.mBytesPerPacket);
    Com_Printf("  %5d mFramesPerPacket\n", outputStreamBasicDescription.mFramesPerPacket);
    Com_Printf("  %5d mBytesPerFrame\n", outputStreamBasicDescription.mBytesPerFrame);
    Com_Printf("  %5d mChannelsPerFrame\n", outputStreamBasicDescription.mChannelsPerFrame);
    Com_Printf("  %5d mBitsPerChannel\n", outputStreamBasicDescription.mBitsPerChannel);

    if(outputStreamBasicDescription.mFormatID != kAudioFormatLinearPCM) {
            Com_Printf("Default Audio Device doesn't support Linear PCM!");
            return false;
    }
    
    // Start sound running
	status = AudioDeviceCreateIOProcID(outputDeviceID, audioDeviceIOProc, NULL, &outputDeviceProcID);
    if (status) {
        Com_Printf("AudioDeviceAddIOProc: returned %d\n", status);
        return false;
    }

    submissionChunk = chunkSize->value;
	maxMixedSamples = bufferSize->value;
    s_mixedSamples = calloc(1, sizeof(*s_mixedSamples) * maxMixedSamples);
    Com_Printf("Chunk Count = %d\n", (maxMixedSamples / submissionChunk));
    
    // Tell the main app what we expect from it
    dma.samples = maxMixedSamples;
    dma.submission_chunk = submissionChunk;
    dma.samplebits = 16;
    dma.buffer = (byte *)s_mixedSamples;
    dma.channels = outputStreamBasicDescription.mChannelsPerFrame;
    dma.speed = (unsigned long)outputStreamBasicDescription.mSampleRate;

    // We haven't enqueued anything yet
    s_chunkCount = 0;

    status = AudioDeviceStart(outputDeviceID, audioDeviceIOProc);
    if (status) {
        Com_Printf("AudioDeviceStart: returned %d\n", status);
        return false;
    }

    s_isRunning = true;
    
    return true;
}

/*
===============
SNDDMA_GetBufferDuration
===============
*/
float SNDDMA_GetBufferDuration(void)
{
    return (float)dma.samples / (float)(dma.channels * dma.speed);
}

/*
===============
SNDDMA_GetDMAPos
===============
*/
int SNDDMA_GetDMAPos(void)
{
    return s_chunkCount * dma.submission_chunk;
}

/*
===============
SNDDMA_Shutdown
===============
*/
void SNDDMA_Shutdown(void)
{
    OSStatus status;
    
    if (!s_isRunning)
        return;
        
    status = AudioDeviceStop(outputDeviceID, audioDeviceIOProc);
    if (status) {
        Com_Printf("AudioDeviceStop: returned %d\n", status);
        return;
    }
    
    s_isRunning = false;

	status = AudioDeviceDestroyIOProcID(outputDeviceID, outputDeviceProcID);
    if (status) {
        Com_Printf("AudioDeviceRemoveIOProc: returned %d\n", status);
        return;
    }
    
    free(s_mixedSamples);
    s_mixedSamples = NULL;
    dma.samples = 0;
}

/*
===============
SNDDMA_BeginPainting
===============
*/
void SNDDMA_BeginPainting(void) {
}

/*
===============
SNDDMA_Submit
===============
*/
void SNDDMA_Submit(void) {
}

