//==========================================================================================
// FMOD Main header file. Copyright (c), FireLight Multimedia 1999-2001.
//==========================================================================================

#ifndef _FMOD_H_
#define _FMOD_H_

//===============================================================================================
// DEFINITIONS
//===============================================================================================

#if defined(__GNUC__) && defined(WIN32)
       #define _cdecl
#endif /* defined(__GNUC__) && defined(WIN32) */

#if defined(PLATFORM_LINUX)
       #define _cdecl 
       #define _stdcall
       #define __cdecl 
       #define __stdcall
       #define __declspec(x)
       #define __PS __attribute__((packed)) /* gcc packed */
#else 
       #define __PS /*dummy*/
#endif 

#define F_API _stdcall

#ifdef DLL_EXPORTS
	#define DLL_API __declspec(dllexport)
#else
	#ifdef __LCC__
		#define DLL_API F_API
	#else
		#define DLL_API
	#endif // __LCC__
#endif //DLL_EXPORTS

#define FMOD_VERSION	3.31f

// fmod defined types
typedef struct FSOUND_MATERIAL  FSOUND_MATERIAL;
typedef struct FSOUND_GEOMLIST  FSOUND_GEOMLIST;
typedef struct FSOUND_SAMPLE	FSOUND_SAMPLE;
typedef struct FSOUND_STREAM	FSOUND_STREAM;
typedef struct FSOUND_DSPUNIT	FSOUND_DSPUNIT;
typedef struct FMUSIC_MODULE	FMUSIC_MODULE;

// callback types
typedef signed char (_cdecl *FSOUND_STREAMCALLBACK)	(FSOUND_STREAM *stream, void *buff, int len, int param);
typedef void *		(_cdecl *FSOUND_DSPCALLBACK)	(void *originalbuffer, void *newbuffer, int length, int param);
typedef void		(_cdecl *FMUSIC_CALLBACK)		(FMUSIC_MODULE *mod, unsigned char param);

/*
[ENUM]
[
	[DESCRIPTION]	
	On failure of commands in FMOD, use FSOUND_GetError to attain what happened.
	
	[SEE_ALSO]		
	FSOUND_GetError
]
*/
enum FMOD_ERRORS 
{
	FMOD_ERR_NONE,			   // No errors
	FMOD_ERR_BUSY,             // Cannot call this command after FSOUND_Init.  Call FSOUND_Close first.
	FMOD_ERR_UNINITIALIZED,	   // This command failed because FSOUND_Init was not called
	FMOD_ERR_INIT,			   // Error initializing output device.
	FMOD_ERR_ALLOCATED,		   // Error initializing output device, but more specifically, the output device is already in use and cannot be reused.
	FMOD_ERR_PLAY,			   // Playing the sound failed.
	FMOD_ERR_OUTPUT_FORMAT,	   // Soundcard does not support the features needed for this soundsystem (16bit stereo output)
	FMOD_ERR_COOPERATIVELEVEL, // Error setting cooperative level for hardware.
	FMOD_ERR_CREATEBUFFER,	   // Error creating hardware sound buffer.
	FMOD_ERR_FILE_NOTFOUND,	   // File not found
	FMOD_ERR_FILE_FORMAT,	   // Unknown file format
	FMOD_ERR_FILE_BAD,		   // Error loading file
	FMOD_ERR_MEMORY,           // Not enough memory 
	FMOD_ERR_VERSION,		   // The version number of this file format is not supported
	FMOD_ERR_INVALID_PARAM,	   // An invalid parameter was passed to this function
	FMOD_ERR_NO_EAX,		   // Tried to use an EAX command on a non EAX enabled channel or output.
	FMOD_ERR_NO_EAX2,		   // Tried to use an advanced EAX2 command on a non EAX2 enabled channel or output.
	FMOD_ERR_CHANNEL_ALLOC,	   // Failed to allocate a new channel
	FMOD_ERR_RECORD,		   // Recording is not supported on this machine
	FMOD_ERR_MEDIAPLAYER,	   // Windows Media Player not installed so cant play wma or use internet streaming.
};


/*
[ENUM]
[
	[DESCRIPTION]	
	These output types are used with FSOUND_SetOutput, to choose which output driver to use.
	
	FSOUND_OUTPUT_A3D will cause FSOUND_Init to FAIL if you have not got a vortex 
	based A3D card.  The suggestion for this is to immediately try and reinitialize FMOD with
	FSOUND_OUTPUT_DSOUND, and if this fails, try initializing FMOD with	FSOUND_OUTPUT_WAVEOUT.
	
	FSOUND_OUTPUT_DSOUND will not support hardware 3d acceleration if the sound card driver 
	does not support DirectX 6 Voice Manager Extensions.

	[SEE_ALSO]		
	FSOUND_SetOutput
	FSOUND_GetOutput
]
*/
enum FSOUND_OUTPUTTYPES
{
	FSOUND_OUTPUT_NOSOUND,    // NoSound driver, all calls to this succeed but do nothing.
	FSOUND_OUTPUT_WINMM,      // Windows Multimedia driver.
	FSOUND_OUTPUT_DSOUND,     // DirectSound driver.  You need this to get EAX or EAX2 support.
	FSOUND_OUTPUT_A3D,        // A3D driver.  You need this to get geometry support.
    FSOUND_OUTPUT_OSS,        // Linux/Unix OSS (Open Sound System) driver, i.e. the kernel sound drivers.
    FSOUND_OUTPUT_ESD,        // Linux/Unix ESD (Enlightment Sound Daemon) driver.
    FSOUND_OUTPUT_ALSA        // Linux Alsa driver.
};


/*
[ENUM]
[
	[DESCRIPTION]	
	These mixer types are used with FSOUND_SetMixer, to choose which mixer to use, or to act 
	upon for other reasons using FSOUND_GetMixer.

	[SEE_ALSO]		
	FSOUND_SetMixer
	FSOUND_GetMixer
]
*/
enum FSOUND_MIXERTYPES
{
	FSOUND_MIXER_AUTODETECT,		// Enables autodetection of the fastest mixer based on your cpu.
	FSOUND_MIXER_BLENDMODE,			// Enables the standard non mmx, blendmode mixer.
	FSOUND_MIXER_MMXP5,				// Enables the mmx, pentium optimized blendmode mixer.
	FSOUND_MIXER_MMXP6,				// Enables the mmx, ppro/p2/p3 optimized mixer.

	FSOUND_MIXER_QUALITY_AUTODETECT,// Enables autodetection of the fastest quality mixer based on your cpu.
	FSOUND_MIXER_QUALITY_FPU,		// Enables the interpolating/volume ramping FPU mixer. 
	FSOUND_MIXER_QUALITY_MMXP5,		// Enables the interpolating/volume ramping p5 MMX mixer. 
	FSOUND_MIXER_QUALITY_MMXP6,		// Enables the interpolating/volume ramping ppro/p2/p3+ MMX mixer. 
};

/*
[ENUM]
[
	[DESCRIPTION]	
	These definitions describe the type of song being played.

	[SEE_ALSO]		
	FMUSIC_GetType	
]
*/
enum FMUSIC_TYPES
{
	FMUSIC_TYPE_NONE,		
	FMUSIC_TYPE_MOD,		// Protracker / Fasttracker
	FMUSIC_TYPE_S3M,		// ScreamTracker 3
	FMUSIC_TYPE_XM,			// FastTracker 2
	FMUSIC_TYPE_IT,			// Impulse Tracker.
	FMUSIC_TYPE_MIDI,		// MIDI file
};


/*
[DEFINE_START] 
[
 	[NAME] 
	FSOUND_DSP_PRIORITIES

	[DESCRIPTION]	
	These default priorities are 

	[SEE_ALSO]		
	FSOUND_DSP_Create
	FSOUND_DSP_SetPriority
]
*/
#define FSOUND_DSP_DEFAULTPRIORITY_CLEARUNIT		0		// DSP CLEAR unit - done first
#define FSOUND_DSP_DEFAULTPRIORITY_SFXUNIT			100		// DSP SFX unit - done second
#define FSOUND_DSP_DEFAULTPRIORITY_MUSICUNIT		200		// DSP MUSIC unit - done third
#define FSOUND_DSP_DEFAULTPRIORITY_USER				300		// User priority, use this as reference
#define FSOUND_DSP_DEFAULTPRIORITY_CLIPANDCOPYUNIT	1000	// DSP CLIP AND COPY unit - last
// [DEFINE_END]


/*
[DEFINE_START] 
[
 	[NAME] 
	FSOUND_CAPS

	[DESCRIPTION]	
	Driver description bitfields.  Use FSOUND_Driver_GetCaps to determine if a driver enumerated
	has the settings you are after.  The enumerated driver depends on the output mode, see
	FSOUND_OUTPUTTYPES

	[SEE_ALSO]
	FSOUND_GetDriverCaps
	FSOUND_OUTPUTTYPES
]
*/
#define FSOUND_CAPS_HARDWARE				0x1		// This driver supports hardware accelerated 3d sound.
#define FSOUND_CAPS_EAX						0x2		// This driver supports EAX reverb
#define FSOUND_CAPS_GEOMETRY_OCCLUSIONS		0x4		// This driver supports (A3D) geometry occlusions
#define FSOUND_CAPS_GEOMETRY_REFLECTIONS	0x8		// This driver supports (A3D) geometry reflections
#define FSOUND_CAPS_EAX2					0x10	// This driver supports EAX2/A3D3 reverb
// [DEFINE_END]


/*
[DEFINE_START] 
[
 	[NAME] 
	FSOUND_MODES
	
	[DESCRIPTION]	
	Sample description bitfields, OR them together for loading and describing samples.
]
*/
#define FSOUND_LOOP_OFF		0x00000001	// For non looping samples.
#define FSOUND_LOOP_NORMAL	0x00000002	// For forward looping samples.
#define FSOUND_LOOP_BIDI	0x00000004	// For bidirectional looping samples.  (no effect if in hardware).
#define FSOUND_8BITS		0x00000008	// For 8 bit samples.
#define FSOUND_16BITS		0x00000010	// For 16 bit samples.
#define FSOUND_MONO			0x00000020	// For mono samples.
#define FSOUND_STEREO		0x00000040	// For stereo samples.
#define FSOUND_UNSIGNED		0x00000080	// For source data containing unsigned samples.
#define FSOUND_SIGNED		0x00000100	// For source data containing signed data.
#define FSOUND_DELTA		0x00000200	// For source data stored as delta values.
#define FSOUND_IT214		0x00000400	// For source data stored using IT214 compression.
#define FSOUND_IT215		0x00000800	// For source data stored using IT215 compression.
#define FSOUND_HW3D			0x00001000	// Attempts to make samples use 3d hardware acceleration. (if the card supports it)
#define FSOUND_2D			0x00002000	// Ignores any 3d processing.  overrides FSOUND_HW3D.  Located in software.
#define FSOUND_STREAMABLE	0x00004000	// For a streamable sound where you feed the data to it.  If you dont supply this sound may come out corrupted.  (only affects a3d output)
#define FSOUND_LOADMEMORY	0x00008000	// 'name' will be interpreted as a pointer to data for streaming and samples.
#define FSOUND_LOADRAW		0x00010000	// For  will ignore file format and treat as raw pcm.
#define FSOUND_MPEGACCURATE	0x00020000	// For FSOUND_Stream_OpenFile - for accurate FSOUND_Stream_GetLengthMs/FSOUND_Stream_SetTime.  WARNING, see FSOUNDStream_OpenFile for inital opening time performance issues.

// Default sample type.  Loop off, 8bit mono, signed, not hardware accelerated.  
// Some API functions ignore 8bits and mono, as it may be an mpeg/wav/etc which has its format predetermined.
#define FSOUND_NORMAL		(FSOUND_LOOP_OFF | FSOUND_8BITS | FSOUND_MONO)		
// [DEFINE_END]


/*
[DEFINE_START] 
[
 	[NAME] 
	FSOUND_CDPLAYMODES
	
	[DESCRIPTION]	
	Playback method for a CD Audio track, using FSOUND_CD_Play

	[SEE_ALSO]		
	FSOUND_CD_Play
]
*/
#define FSOUND_CD_PLAYCONTINUOUS	0	// Starts from the current track and plays to end of CD.
#define FSOUND_CD_PLAYONCE			1	// Plays the specified track then stops.
#define FSOUND_CD_PLAYLOOPED		2	// Plays the specified track looped, forever until stopped manually.
#define FSOUND_CD_PLAYRANDOM		3	// Plays tracks in random order
// [DEFINE_END]


/*
[DEFINE_START] 
[
 	[NAME] 
	FSOUND_CHANNELSAMPLEMODE
	
	[DESCRIPTION]
	Miscellaneous values for FMOD functions.

	[SEE_ALSO]
	FSOUND_PlaySound
	FSOUND_PlaySound3DAttrib

	FSOUND_Sample_Alloc
	FSOUND_Sample_Load
	FSOUND_SetPan
]
*/
#define FSOUND_FREE			-1	// value to play on any free channel, or to allocate a sample in a free sample slot.
#define FSOUND_UNMANAGED	-2	// value to allocate a sample that is NOT managed by FSOUND or placed in a sample slot.
#define FSOUND_ALL			-3	// for a channel index , this flag will affect ALL channels available!  Not supported by every function.
#define FSOUND_STEREOPAN	-1	// value for FSOUND_SetPan so that stereo sounds are not played at half volume.  See FSOUND_SetPan for more on this.
// [DEFINE_END]


/*
[ENUM]
[
	[DESCRIPTION]	
	These are environment types defined for use with the FSOUND_Reverb API.

	[SEE_ALSO]		
	FSOUND_Reverb_SetEnvironment
	FSOUND_Reverb_SetEnvironmentAdvanced
]
*/
enum FSOUND_REVERB_ENVIRONMENTS
{
    FSOUND_ENVIRONMENT_GENERIC,
    FSOUND_ENVIRONMENT_PADDEDCELL,
    FSOUND_ENVIRONMENT_ROOM,
    FSOUND_ENVIRONMENT_BATHROOM,
    FSOUND_ENVIRONMENT_LIVINGROOM,
    FSOUND_ENVIRONMENT_STONEROOM,
    FSOUND_ENVIRONMENT_AUDITORIUM,
    FSOUND_ENVIRONMENT_CONCERTHALL,
    FSOUND_ENVIRONMENT_CAVE,
    FSOUND_ENVIRONMENT_ARENA,
    FSOUND_ENVIRONMENT_HANGAR,
    FSOUND_ENVIRONMENT_CARPETEDHALLWAY,
    FSOUND_ENVIRONMENT_HALLWAY,
    FSOUND_ENVIRONMENT_STONECORRIDOR,
    FSOUND_ENVIRONMENT_ALLEY,
    FSOUND_ENVIRONMENT_FOREST,
    FSOUND_ENVIRONMENT_CITY,
    FSOUND_ENVIRONMENT_MOUNTAINS,
    FSOUND_ENVIRONMENT_QUARRY,
    FSOUND_ENVIRONMENT_PLAIN,
    FSOUND_ENVIRONMENT_PARKINGLOT,
    FSOUND_ENVIRONMENT_SEWERPIPE,
    FSOUND_ENVIRONMENT_UNDERWATER,
    FSOUND_ENVIRONMENT_DRUGGED,
    FSOUND_ENVIRONMENT_DIZZY,
    FSOUND_ENVIRONMENT_PSYCHOTIC,

    FSOUND_ENVIRONMENT_COUNT
};

/*
[DEFINE_START] 
[
 	[NAME] 
	FSOUND_REVERBMIX_USEDISTANCE
	
	[DESCRIPTION]	
	Used with FSOUND_Reverb_SetMix, this setting allows reverb to attenuate based on distance from the listener.
	Instead of hard coding a value with FSOUND_Reverb_SetMix, this value can be used instead, for a more natural
	reverb dropoff.

	[SEE_ALSO]
	FSOUND_Reverb_SetMix
]
*/
#define FSOUND_REVERBMIX_USEDISTANCE	-1.0f	// used with FSOUND_Reverb_SetMix to scale reverb by distance
// [DEFINE_END]


/*
[DEFINE_START] 
[
 	[NAME] 
	FSOUND_REVERB_IGNOREPARAM
	
	[DESCRIPTION]	
	Used with FSOUND_Reverb_SetEnvironment and FSOUND_Reverb_SetEnvironmentAdvanced, this can
	be placed in the place of a specific parameter for the reverb setting.  It allows you to 
	not set any parameters except the ones you are interested in .. and example would be this.
	FSOUND_Reverb_SetEnvironment(FSOUND_REVERB_IGNOREPARAM,
								 FSOUND_REVERB_IGNOREPARAM,
								 FSOUND_REVERB_IGNOREPARAM,  
								 0.0f);
	This means env, vol and decay are left alone, but 'damp' is set to 0.

	[SEE_ALSO]
	FSOUND_Reverb_SetEnvironment
	FSOUND_Reverb_SetEnvironmentAdvanced
]
*/
#define FSOUND_REVERB_IGNOREPARAM	-9999999	// used with FSOUND_Reverb_SetEnvironmentAdvanced to ignore certain parameters by choice.
// [DEFINE_END]


/*
[DEFINE_START] 
[
 	[NAME] 
	FSOUND_REVERB_PRESETS
	
	[DESCRIPTION]	
	A set of predefined environment PARAMETERS, created by Creative Labs
	These can be placed directly into the FSOUND_Reverb_SetEnvironment call

	[SEE_ALSO]
	FSOUND_Reverb_SetEnvironment
]
*/
#define FSOUND_PRESET_OFF			  FSOUND_ENVIRONMENT_GENERIC,0.0f,0.0f,0.0f
#define FSOUND_PRESET_GENERIC         FSOUND_ENVIRONMENT_GENERIC,0.5f,1.493f,0.5f
#define FSOUND_PRESET_PADDEDCELL      FSOUND_ENVIRONMENT_PADDEDCELL,0.25f,0.1f,0.0f
#define FSOUND_PRESET_ROOM            FSOUND_ENVIRONMENT_ROOM,0.417f,0.4f,0.666f
#define FSOUND_PRESET_BATHROOM        FSOUND_ENVIRONMENT_BATHROOM,0.653f,1.499f,0.166f
#define FSOUND_PRESET_LIVINGROOM      FSOUND_ENVIRONMENT_LIVINGROOM,0.208f,0.478f,0.0f
#define FSOUND_PRESET_STONEROOM       FSOUND_ENVIRONMENT_STONEROOM,0.5f,2.309f,0.888f
#define FSOUND_PRESET_AUDITORIUM      FSOUND_ENVIRONMENT_AUDITORIUM,0.403f,4.279f,0.5f
#define FSOUND_PRESET_CONCERTHALL     FSOUND_ENVIRONMENT_CONCERTHALL,0.5f,3.961f,0.5f
#define FSOUND_PRESET_CAVE            FSOUND_ENVIRONMENT_CAVE,0.5f,2.886f,1.304f
#define FSOUND_PRESET_ARENA           FSOUND_ENVIRONMENT_ARENA,0.361f,7.284f,0.332f
#define FSOUND_PRESET_HANGAR          FSOUND_ENVIRONMENT_HANGAR,0.5f,10.0f,0.3f
#define FSOUND_PRESET_CARPETEDHALLWAY FSOUND_ENVIRONMENT_CARPETEDHALLWAY,0.153f,0.259f,2.0f
#define FSOUND_PRESET_HALLWAY         FSOUND_ENVIRONMENT_HALLWAY,0.361f,1.493f,0.0f
#define FSOUND_PRESET_STONECORRIDOR   FSOUND_ENVIRONMENT_STONECORRIDOR,0.444f,2.697f,0.638f
#define FSOUND_PRESET_ALLEY           FSOUND_ENVIRONMENT_ALLEY,0.25f,1.752f,0.776f
#define FSOUND_PRESET_FOREST          FSOUND_ENVIRONMENT_FOREST,0.111f,3.145f,0.472f
#define FSOUND_PRESET_CITY            FSOUND_ENVIRONMENT_CITY,0.111f,2.767f,0.224f
#define FSOUND_PRESET_MOUNTAINS       FSOUND_ENVIRONMENT_MOUNTAINS,0.194f,7.841f,0.472f
#define FSOUND_PRESET_QUARRY          FSOUND_ENVIRONMENT_QUARRY,1.0f,1.499f,0.5f
#define FSOUND_PRESET_PLAIN           FSOUND_ENVIRONMENT_PLAIN,0.097f,2.767f,0.224f
#define FSOUND_PRESET_PARKINGLOT      FSOUND_ENVIRONMENT_PARKINGLOT,0.208f,1.652f,1.5f
#define FSOUND_PRESET_SEWERPIPE       FSOUND_ENVIRONMENT_SEWERPIPE,0.652f,2.886f,0.25f
#define FSOUND_PRESET_UNDERWATER      FSOUND_ENVIRONMENT_UNDERWATER,1.0f,1.499f,0.0f
#define FSOUND_PRESET_DRUGGED         FSOUND_ENVIRONMENT_DRUGGED,0.875f, 8.392f,1.388f
#define FSOUND_PRESET_DIZZY           FSOUND_ENVIRONMENT_DIZZY,0.139f,17.234f,0.666f
#define FSOUND_PRESET_PSYCHOTIC       FSOUND_ENVIRONMENT_PSYCHOTIC,0.486f,7.563f,0.806f
// [DEFINE_END]


/*
[DEFINE_START] 
[
 	[NAME] 
	FSOUND_GEOMETRY_MODES
	
	[DESCRIPTION]	
	Geometry flags, used as the mode flag in FSOUND_Geometry_AddPolygon

	[SEE_ALSO]
	FSOUND_Geometry_AddPolygon
]
*/
#define FSOUND_GEOMETRY_NORMAL				0x0		// Default geometry type.  Occluding polygon
#define FSOUND_GEOMETRY_REFLECTIVE			0x01	// This polygon is reflective
#define FSOUND_GEOMETRY_OPENING				0x02	// Overlays a transparency over the previous polygon.  The 'openingfactor' value supplied is copied internally.
#define FSOUND_GEOMETRY_OPENING_REFERENCE	0x04	// Overlays a transparency over the previous polygon.  The 'openingfactor' supplied is pointed to (for access when building a list)
// [DEFINE_END]

/*
[DEFINE_START] 
[
 	[NAME] 
	FSOUND_INIT_FLAGS
	
	[DESCRIPTION]	
	Initialization flags.  Use them with FSOUND_Init in the flags parameter to change various behaviour.

	[SEE_ALSO]
	FSOUND_Init
]
*/
#define FSOUND_INIT_USEDEFAULTMIDISYNTH		0x01	// Causes MIDI playback to force software decoding.
// [DEFINE_END]




//===============================================================================================
// FUNCTION PROTOTYPES
//===============================================================================================

/* Lazarus: We use GetProcAddress to resolve FMOD references, so these aren't needed (and
   cause conflicts if we try to use the same names.

#ifdef __cplusplus
extern "C" {
#endif

// ==================================
// Initialization / Global functions.
// ==================================

// *Pre* FSOUND_Init functions. These can't be called after FSOUND_Init is 
// called (they will fail). They set up FMOD system functionality. 
DLL_API signed char		F_API FSOUND_SetOutput(int outputtype);
DLL_API signed char		F_API FSOUND_SetDriver(int driver);
DLL_API signed char		F_API FSOUND_SetMixer(int mixer);
DLL_API signed char		F_API FSOUND_SetBufferSize(int len_ms);
DLL_API signed char		F_API FSOUND_SetHWND(void *hwnd);
DLL_API	signed char		F_API FSOUND_SetMinHardwareChannels(int min);
DLL_API	signed char		F_API FSOUND_SetMaxHardwareChannels(int max);

// Main initialization / closedown functions.
// Note : Use FSOUND_INIT_USEDEFAULTMIDISYNTH with FSOUND_Init for software override with MIDI playback.
DLL_API signed char		F_API FSOUND_Init(int mixrate, int maxsoftwarechannels, unsigned int flags);
DLL_API void			F_API FSOUND_Close();

// Runtime
DLL_API void			F_API FSOUND_SetSFXMasterVolume(int volume);
DLL_API void			F_API FSOUND_SetPanSeperation(float pansep);

// System information.
DLL_API int				F_API FSOUND_GetError();
DLL_API float			F_API FSOUND_GetVersion();
DLL_API int				F_API FSOUND_GetOutput();
DLL_API int				F_API FSOUND_GetDriver();
DLL_API int				F_API FSOUND_GetMixer();
DLL_API int				F_API FSOUND_GetNumDrivers();
DLL_API signed char *	F_API FSOUND_GetDriverName(int id);
DLL_API signed char 	F_API FSOUND_GetDriverCaps(int id, unsigned int *caps);
DLL_API int				F_API FSOUND_GetOutputRate();
DLL_API int				F_API FSOUND_GetMaxChannels();
DLL_API int				F_API FSOUND_GetMaxSamples();
DLL_API int				F_API FSOUND_GetSFXMasterVolume();
DLL_API int				F_API FSOUND_GetNumHardwareChannels();
DLL_API int				F_API FSOUND_GetChannelsPlaying();
DLL_API float			F_API FSOUND_GetCPUUsage();

// ===================================
// Sample management / load functions.
// ===================================

// Note : Use FSOUND_LOADMEMORY   flag with FSOUND_Sample_Load to load from memory.
//        Use FSOUND_LOADRAW      flag with FSOUND_Sample_Load to treat as as raw pcm data.

// Sample creation and management functions
DLL_API FSOUND_SAMPLE *	F_API FSOUND_Sample_Load(int index, const char *name, unsigned int mode, int memlength);
DLL_API FSOUND_SAMPLE *	F_API FSOUND_Sample_Alloc(int index, int length, unsigned int mode, int deffreq, int defvol, int defpan, int defpri);
DLL_API void			F_API FSOUND_Sample_Free(FSOUND_SAMPLE *sptr);
DLL_API signed char		F_API FSOUND_Sample_Upload(FSOUND_SAMPLE *sptr, void *srcdata, unsigned int mode);
DLL_API signed char		F_API FSOUND_Sample_Lock(FSOUND_SAMPLE *sptr, int offset, int length, void **ptr1, void **ptr2, unsigned int *len1, unsigned int *len2);
DLL_API signed char		F_API FSOUND_Sample_Unlock(FSOUND_SAMPLE *sptr, void *ptr1, void *ptr2, unsigned int len1, unsigned int len2);

// Sample control functions
DLL_API signed char		F_API FSOUND_Sample_SetLoopMode(FSOUND_SAMPLE *sptr, unsigned int loopmode);
DLL_API signed char		F_API FSOUND_Sample_SetLoopPoints(FSOUND_SAMPLE *sptr, int loopstart, int loopend);
DLL_API signed char		F_API FSOUND_Sample_SetDefaults(FSOUND_SAMPLE *sptr, int deffreq, int defvol, int defpan, int defpri);
DLL_API signed char		F_API FSOUND_Sample_SetMinMaxDistance(FSOUND_SAMPLE *sptr, float min, float max);

// Sample information 
DLL_API FSOUND_SAMPLE * F_API FSOUND_Sample_Get(int sampno);
DLL_API char *			F_API FSOUND_Sample_GetName(FSOUND_SAMPLE *sptr);
DLL_API unsigned int	F_API FSOUND_Sample_GetLength(FSOUND_SAMPLE *sptr);
DLL_API signed char		F_API FSOUND_Sample_GetLoopPoints(FSOUND_SAMPLE *sptr, int *loopstart, int *loopend);
DLL_API signed char		F_API FSOUND_Sample_GetDefaults(FSOUND_SAMPLE *sptr, int *deffreq, int *defvol, int *defpan, int *defpri);
DLL_API unsigned int	F_API FSOUND_Sample_GetMode(FSOUND_SAMPLE *sptr);
  
// ============================
// Channel control functions.
// ============================

// Playing and stopping sounds.
DLL_API int				F_API FSOUND_PlaySound(int channel, FSOUND_SAMPLE *sptr);
DLL_API int				F_API FSOUND_PlaySound3DAttrib(int channel, FSOUND_SAMPLE *sptr, int freq, int vol, int pan, float *pos, float *vel);
DLL_API signed char		F_API FSOUND_StopSound(int channel);
 
// Functions to control playback of a channel.
DLL_API signed char		F_API FSOUND_SetFrequency(int channel, int freq);
DLL_API signed char		F_API FSOUND_SetVolume(int channel, int vol);
DLL_API signed char 	F_API FSOUND_SetVolumeAbsolute(int channel, int vol);
DLL_API signed char		F_API FSOUND_SetPan(int channel, int pan);
DLL_API signed char		F_API FSOUND_SetSurround(int channel, signed char surround);
DLL_API signed char		F_API FSOUND_SetMute(int channel, signed char mute);
DLL_API signed char		F_API FSOUND_SetPriority(int channel, int priority);
DLL_API signed char		F_API FSOUND_SetReserved(int channel, signed char reserved);
DLL_API signed char		F_API FSOUND_SetPaused(int channel, signed char paused);
DLL_API signed char		F_API FSOUND_SetLoopMode(int channel, unsigned int loopmode);

// Channel information
DLL_API signed char		F_API FSOUND_IsPlaying(int channel);
DLL_API int				F_API FSOUND_GetFrequency(int channel);
DLL_API int				F_API FSOUND_GetVolume(int channel);
DLL_API int				F_API FSOUND_GetPan(int channel);
DLL_API signed char		F_API FSOUND_GetSurround(int channel);
DLL_API signed char		F_API FSOUND_GetMute(int channel);
DLL_API int				F_API FSOUND_GetPriority(int channel);
DLL_API signed char		F_API FSOUND_GetReserved(int channel);
DLL_API signed char		F_API FSOUND_GetPaused(int channel);
DLL_API unsigned int	F_API FSOUND_GetCurrentPosition(int channel);
DLL_API FSOUND_SAMPLE *	F_API FSOUND_GetCurrentSample(int channel);
DLL_API float			F_API FSOUND_GetCurrentVU(int channel);
 
// ===================
// 3D sound functions.
// ===================
// see also FSOUND_PlaySound3DAttrib (above)
// see also FSOUND_Sample_SetMinMaxDistance (above)
DLL_API void			F_API FSOUND_3D_Update();
DLL_API signed char		F_API FSOUND_3D_SetAttributes(int channel, float *pos, float *vel);
DLL_API signed char		F_API FSOUND_3D_GetAttributes(int channel, float *pos, float *vel);
DLL_API void			F_API FSOUND_3D_Listener_SetAttributes(float *pos, float *vel, float fx, float fy, float fz, float tx, float ty, float tz);
DLL_API void			F_API FSOUND_3D_Listener_GetAttributes(float *pos, float *vel, float *fx, float *fy, float *fz, float *tx, float *ty, float *tz);
DLL_API void			F_API FSOUND_3D_Listener_SetDopplerFactor(float scale);
DLL_API void			F_API FSOUND_3D_Listener_SetDistanceFactor(float scale);
DLL_API void			F_API FSOUND_3D_Listener_SetRolloffFactor(float scale);
 
// =========================
// File Streaming functions.
// =========================

// Note : Use FSOUND_LOADMEMORY   flag with FSOUND_Stream_OpenFile to stream from memory.
//        Use FSOUND_LOADRAW      flag with FSOUND_Stream_OpenFile to treat stream as raw pcm data.
//        Use FSOUND_MPEGACCURATE flag with FSOUND_Stream_OpenFile to open mpegs in 'accurate mode' for seeking etc.

DLL_API FSOUND_STREAM *	F_API FSOUND_Stream_OpenFile(const char *filename, unsigned int mode, int memlength);
DLL_API FSOUND_STREAM *	F_API FSOUND_Stream_Create(FSOUND_STREAMCALLBACK callback, int length, unsigned int mode, int samplerate, int userdata);
DLL_API int 			F_API FSOUND_Stream_Play(int channel, FSOUND_STREAM *stream);
DLL_API int				F_API FSOUND_Stream_Play3DAttrib(int channel, FSOUND_STREAM *stream, int freq, int vol, int pan, float *pos, float *vel);
DLL_API signed char		F_API FSOUND_Stream_Stop(FSOUND_STREAM *stream);
DLL_API signed char		F_API FSOUND_Stream_Close(FSOUND_STREAM *stream);
DLL_API signed char		F_API FSOUND_Stream_SetEndCallback(FSOUND_STREAM *stream, FSOUND_STREAMCALLBACK callback, int userdata);
DLL_API signed char		F_API FSOUND_Stream_SetSynchCallback(FSOUND_STREAM *stream, FSOUND_STREAMCALLBACK callback, int userdata);
DLL_API FSOUND_SAMPLE * F_API FSOUND_Stream_GetSample(FSOUND_STREAM *stream);
DLL_API FSOUND_DSPUNIT *F_API FSOUND_Stream_CreateDSP(FSOUND_STREAM *stream, FSOUND_DSPCALLBACK callback, int priority, int param);

DLL_API signed char		F_API FSOUND_Stream_SetPaused(FSOUND_STREAM *stream, signed char paused);
DLL_API signed char		F_API FSOUND_Stream_GetPaused(FSOUND_STREAM *stream);
DLL_API signed char		F_API FSOUND_Stream_SetPosition(FSOUND_STREAM *stream, int position);
DLL_API int				F_API FSOUND_Stream_GetPosition(FSOUND_STREAM *stream);
DLL_API signed char		F_API FSOUND_Stream_SetTime(FSOUND_STREAM *stream, int ms);
DLL_API int				F_API FSOUND_Stream_GetTime(FSOUND_STREAM *stream);
DLL_API int				F_API FSOUND_Stream_GetLength(FSOUND_STREAM *stream);
DLL_API int				F_API FSOUND_Stream_GetLengthMs(FSOUND_STREAM *stream);

// ===================
// CD audio functions.
// ===================

DLL_API signed char		F_API FSOUND_CD_Play(int track);
DLL_API void			F_API FSOUND_CD_SetPlayMode(signed char mode);
DLL_API signed char		F_API FSOUND_CD_Stop();
DLL_API signed char		F_API FSOUND_CD_SetPaused(signed char paused);
DLL_API	signed char		F_API FSOUND_CD_SetVolume(int volume);
DLL_API signed char		F_API FSOUND_CD_Eject();

DLL_API signed char		F_API FSOUND_CD_GetPaused();
DLL_API int				F_API FSOUND_CD_GetTrack();
DLL_API int				F_API FSOUND_CD_GetNumTracks();
DLL_API	int				F_API FSOUND_CD_GetVolume();
DLL_API int				F_API FSOUND_CD_GetTrackLength(int track); 
DLL_API int				F_API FSOUND_CD_GetTrackTime();


// ==============
// DSP functions.
// ==============

// DSP Unit control and information functions.
DLL_API FSOUND_DSPUNIT *F_API FSOUND_DSP_Create(FSOUND_DSPCALLBACK callback, int priority, int param);
DLL_API void			F_API FSOUND_DSP_Free(FSOUND_DSPUNIT *unit);
DLL_API void			F_API FSOUND_DSP_SetPriority(FSOUND_DSPUNIT *unit, int priority);
DLL_API int				F_API FSOUND_DSP_GetPriority(FSOUND_DSPUNIT *unit);
DLL_API void			F_API FSOUND_DSP_SetActive(FSOUND_DSPUNIT *unit, signed char active);
DLL_API signed char		F_API FSOUND_DSP_GetActive(FSOUND_DSPUNIT *unit);

// Functions to get hold of FSOUND 'system DSP unit' handles.
DLL_API FSOUND_DSPUNIT *F_API FSOUND_DSP_GetClearUnit();
DLL_API FSOUND_DSPUNIT *F_API FSOUND_DSP_GetSFXUnit();
DLL_API FSOUND_DSPUNIT *F_API FSOUND_DSP_GetMusicUnit();
DLL_API FSOUND_DSPUNIT *F_API FSOUND_DSP_GetClipAndCopyUnit();

// misc DSP functions
DLL_API signed char		F_API FSOUND_DSP_MixBuffers(void *destbuffer, void *srcbuffer, int len, int freq, int vol, int pan, unsigned int mode);
DLL_API void			F_API FSOUND_DSP_ClearMixBuffer();
DLL_API int				F_API FSOUND_DSP_GetBufferLength();

// =============================================
// Geometry functions.  (NOT SUPPORTED IN LINUX)
// =============================================

// scene/polygon functions
DLL_API signed char		F_API FSOUND_Geometry_AddPolygon(float *p1, float *p2, float *p3, float *p4, float *normal, unsigned int mode, float *openingfactor);
DLL_API int				F_API FSOUND_Geometry_AddList(FSOUND_GEOMLIST *geomlist);

// polygon list functions
DLL_API FSOUND_GEOMLIST * F_API FSOUND_Geometry_List_Create(signed char boundingvolume);
DLL_API signed char		F_API FSOUND_Geometry_List_Free(FSOUND_GEOMLIST *geomlist);
DLL_API signed char		F_API FSOUND_Geometry_List_Begin(FSOUND_GEOMLIST *geomlist);
DLL_API signed char		F_API FSOUND_Geometry_List_End(FSOUND_GEOMLIST *geomlist);

// material functions
DLL_API FSOUND_MATERIAL *	F_API FSOUND_Geometry_Material_Create();
DLL_API signed char		F_API FSOUND_Geometry_Material_Free(FSOUND_MATERIAL *material);
DLL_API signed char		F_API FSOUND_Geometry_Material_SetAttributes(FSOUND_MATERIAL *material, float reflectancegain, float reflectancefreq, float transmittancegain, float transmittancefreq);
DLL_API signed char		F_API FSOUND_Geometry_Material_GetAttributes(FSOUND_MATERIAL *material, float *reflectancegain, float *reflectancefreq, float *transmittancegain, float *transmittancefreq);
DLL_API signed char		F_API FSOUND_Geometry_Material_Set(FSOUND_MATERIAL *material);

// ========================================================================
// Reverb functions. (eax, eax2, a3d 3.0 reverb)  (NOT SUPPORTED IN LINUX)
// ========================================================================

// eax1, eax2, a3d 3.0 (use FSOUND_REVERB_PRESETS if you like), (eax2 support through emulation/parameter conversion)
DLL_API signed char		F_API FSOUND_Reverb_SetEnvironment(int env, float vol, float decay, float damp);			
// eax2, a3d 3.0 only, does not work on eax1
DLL_API signed char		F_API FSOUND_Reverb_SetEnvironmentAdvanced(
								int	  env,
								int   Room,					// [-10000, 0]     default: -10000 mB	or use FSOUND_REVERB_IGNOREPARAM
								int   RoomHF,				// [-10000, 0]     default: 0 mB		or use FSOUND_REVERB_IGNOREPARAM
								float RoomRolloffFactor,	// [0.0, 10.0]     default: 0.0			or use FSOUND_REVERB_IGNOREPARAM
								float DecayTime,			// [0.1, 20.0]     default: 1.0 s		or use FSOUND_REVERB_IGNOREPARAM
								float DecayHFRatio,			// [0.1, 2.0]      default: 0.5			or use FSOUND_REVERB_IGNOREPARAM
								int   Reflections,			// [-10000, 1000]  default: -10000 mB	or use FSOUND_REVERB_IGNOREPARAM
								float ReflectionsDelay,		// [0.0, 0.3]      default: 0.02 s		or use FSOUND_REVERB_IGNOREPARAM
								int   Reverb,				// [-10000, 2000]  default: -10000 mB	or use FSOUND_REVERB_IGNOREPARAM
								float ReverbDelay,			// [0.0, 0.1]      default: 0.04 s		or use FSOUND_REVERB_IGNOREPARAM
								float EnvironmentSize,		// [0.0, 100.0]    default: 100.0 %		or use FSOUND_REVERB_IGNOREPARAM
								float EnvironmentDiffusion,	// [0.0, 100.0]    default: 100.0 %		or use FSOUND_REVERB_IGNOREPARAM
								float AirAbsorptionHF);		// [20.0, 20000.0] default: 5000.0 Hz	or use FSOUND_REVERB_IGNOREPARAM

DLL_API signed char		F_API FSOUND_Reverb_SetMix(int channel, float mix);

// information functions
DLL_API signed char		F_API FSOUND_Reverb_GetEnvironment(int *env, float *vol, float *decay, float *damp);			
DLL_API signed char		F_API FSOUND_Reverb_GetEnvironmentAdvanced(
								int   *env,
								int   *Room,				
								int   *RoomHF,				
								float *RoomRolloffFactor,	
								float *DecayTime,			
								float *DecayHFRatio,		
								int   *Reflections,			
								float *ReflectionsDelay,	
								int   *Reverb,				
								float *ReverbDelay,			
								float *EnvironmentSize,		
								float *EnvironmentDiffusion,
								float *AirAbsorptionHF);	
DLL_API signed char		F_API FSOUND_Reverb_GetMix(int channel, float *mix);

// =============================================
// Recording functions  (NOT SUPPORTED IN LINUX)
// =============================================

// recording initialization functions
DLL_API signed char		F_API FSOUND_Record_SetDriver(int outputtype);
DLL_API int				F_API FSOUND_Record_GetNumDrivers();
DLL_API signed char *	F_API FSOUND_Record_GetDriverName(int id);
DLL_API int				F_API FSOUND_Record_GetDriver();

// recording functionality.  Only one recording session will work at a time
DLL_API signed char		F_API FSOUND_Record_StartSample(FSOUND_SAMPLE *sptr, signed char loop);// record to sample
DLL_API signed char		F_API FSOUND_Record_Stop();	// stop recording
DLL_API int				F_API FSOUND_Record_GetPosition();	// offset in sample, or wav file

// =========================
// File system override
// =========================

DLL_API void F_API FSOUND_File_SetCallbacks(unsigned int (_cdecl *OpenCallback)(const char *name),
                                            void         (_cdecl *CloseCallback)(unsigned int handle),
                                            int          (_cdecl *ReadCallback)(void *buffer, int size, unsigned int handle),
                                            int          (_cdecl *SeekCallback)(unsigned int handle, int pos, signed char mode),
                                            int          (_cdecl *TellCallback)(unsigned int handle));

// =============================================================================================
// FMUSIC API
// =============================================================================================

// Song management / playback functions.
DLL_API FMUSIC_MODULE * F_API FMUSIC_LoadSong(const char *name);
DLL_API FMUSIC_MODULE * F_API FMUSIC_LoadSongMemory(void *data, int length);
DLL_API signed char		F_API FMUSIC_FreeSong(FMUSIC_MODULE *mod);
DLL_API signed char		F_API FMUSIC_PlaySong(FMUSIC_MODULE *mod);
DLL_API signed char		F_API FMUSIC_StopSong(FMUSIC_MODULE *mod);
DLL_API void			F_API FMUSIC_StopAllSongs();
DLL_API signed char		F_API FMUSIC_SetZxxCallback(FMUSIC_MODULE *mod, FMUSIC_CALLBACK callback);
DLL_API signed char		F_API FMUSIC_SetRowCallback(FMUSIC_MODULE *mod, FMUSIC_CALLBACK callback, int rowstep);
DLL_API signed char		F_API FMUSIC_SetOrderCallback(FMUSIC_MODULE *mod, FMUSIC_CALLBACK callback, int orderstep);
DLL_API signed char		F_API FMUSIC_SetInstCallback(FMUSIC_MODULE *mod, FMUSIC_CALLBACK callback, int instrument);
DLL_API signed char		F_API FMUSIC_SetSample(FMUSIC_MODULE *mod, int sampno, FSOUND_SAMPLE *sptr);
DLL_API signed char		F_API FMUSIC_OptimizeChannels(FMUSIC_MODULE *mod, int maxchannels, int minvolume);

// Runtime song functions.
DLL_API signed char		F_API FMUSIC_SetReverb(signed char reverb);				// MIDI only.
DLL_API signed char		F_API FMUSIC_SetOrder(FMUSIC_MODULE *mod, int order);
DLL_API signed char		F_API FMUSIC_SetPaused(FMUSIC_MODULE *mod, signed char pause);
DLL_API signed char		F_API FMUSIC_SetMasterVolume(FMUSIC_MODULE *mod, int volume);
DLL_API signed char		F_API FMUSIC_SetPanSeperation(FMUSIC_MODULE *mod, float pansep);
 
// Static song information functions.
DLL_API char *			F_API FMUSIC_GetName(FMUSIC_MODULE *mod);
DLL_API signed char		F_API FMUSIC_GetType(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetNumOrders(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetNumPatterns(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetNumInstruments(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetNumSamples(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetNumChannels(FMUSIC_MODULE *mod);
DLL_API FSOUND_SAMPLE * F_API FMUSIC_GetSample(FMUSIC_MODULE *mod, int sampno);
DLL_API int				F_API FMUSIC_GetPatternLength(FMUSIC_MODULE *mod, int orderno);
 
// Runtime song information.
DLL_API signed char		F_API FMUSIC_IsFinished(FMUSIC_MODULE *mod);
DLL_API signed char		F_API FMUSIC_IsPlaying(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetMasterVolume(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetGlobalVolume(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetOrder(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetPattern(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetSpeed(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetBPM(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetRow(FMUSIC_MODULE *mod);
DLL_API signed char		F_API FMUSIC_GetPaused(FMUSIC_MODULE *mod);
DLL_API int				F_API FMUSIC_GetTime(FMUSIC_MODULE *mod);

#ifdef __cplusplus
}
#endif

*/

#endif
