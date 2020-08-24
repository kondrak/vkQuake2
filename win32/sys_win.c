/*
Copyright (C) 1997-2001 Id Software, Inc.
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
// sys_win.h

#include "../qcommon/qcommon.h"
#include "winquake.h"
#include "resource.h"
#include <errno.h>
#include <float.h>
#include <fcntl.h>
#include <stdio.h>
#include <direct.h>
#include <io.h>
#include <conio.h>
#include "../win32/conproc.h"

#define MINIMUM_WIN_MEMORY	0x0a00000
#define MAXIMUM_WIN_MEMORY	0x1000000

//#define DEMO

qboolean s_win95;

int			starttime;
int			ActiveApp;
qboolean	Minimized;

static HANDLE		hinput, houtput;

unsigned	sys_msg_time;
unsigned	sys_frame_time;


static HANDLE		qwclsemaphore;

#define	MAX_NUM_ARGVS	128
int			argc;
char		*argv[MAX_NUM_ARGVS];


/*
===============================================================================

SYSTEM IO

===============================================================================
*/


void Sys_Error (char *error, ...)
{
	va_list		argptr;
	char		text[1024];

	CL_Shutdown ();
	Qcommon_Shutdown ();

	va_start (argptr, error);
	vsnprintf (text, 1024, error, argptr);
	va_end (argptr);

	MessageBox(NULL, text, "Error", 0 /* MB_OK */ );

	if (qwclsemaphore)
		CloseHandle (qwclsemaphore);

// shut down QHOST hooks if necessary
	DeinitConProc ();

	exit (1);
}

void Sys_Quit (void)
{
	timeEndPeriod( 1 );

	CL_Shutdown();
	Qcommon_Shutdown ();
	CloseHandle (qwclsemaphore);
	if (dedicated && dedicated->value)
		FreeConsole ();
	else if (debug_console->value)
		FreeConsole();

// shut down QHOST hooks if necessary
	DeinitConProc ();

	exit (0);
}


void WinError (void)
{
	LPVOID lpMsgBuf;

	FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
	);

	// Display the string.
	MessageBox( NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );

	// Free the buffer.
	LocalFree( lpMsgBuf );
}

//================================================================


/*
================
Sys_ScanForCD

================
*/
char *Sys_ScanForCD (void)
{
	static char	cddir[MAX_OSPATH];
	static qboolean	done;
#ifndef DEMO
	char		drive[4];
	FILE		*f;
	char		test[MAX_QPATH];

	if (done)		// don't re-check
		return cddir;

	// no abort/retry/fail errors
	SetErrorMode (SEM_FAILCRITICALERRORS);

	drive[0] = 'c';
	drive[1] = ':';
	drive[2] = '\\';
	drive[3] = 0;

	done = true;

	// scan the drives
	for (drive[0] = 'c' ; drive[0] <= 'z' ; drive[0]++)
	{
		// where activision put the stuff...
		sprintf (cddir, "%sinstall\\data", drive);
		sprintf (test, "%sinstall\\data\\quake2.exe", drive);
		f = fopen(test, "r");
		if (f)
		{
			fclose (f);
			if (GetDriveType (drive) == DRIVE_CDROM)
				return cddir;
		}
	}
#endif

	cddir[0] = 0;
	
	return NULL;
}

/*
================
Sys_CopyProtect

================
*/
void	Sys_CopyProtect (void)
{
#ifndef DEMO
	char	*cddir;

	cddir = Sys_ScanForCD();
	if (!cddir[0])
		Com_Error (ERR_FATAL, "You must have the Quake2 CD in the drive to play.");
#endif
}

/*
================
Sys_SetDPIAwareness

================
*/
typedef enum { dpi_unaware = 0, dpi_system_aware = 1, dpi_monitor_aware = 2 } dpi_awareness;
typedef BOOL(WINAPI *SetProcessDPIAwareFunc)();
typedef HRESULT(WINAPI *SetProcessDPIAwarenessFunc)(dpi_awareness value);

void	Sys_SetDPIAwareness(void)
{
	HMODULE hShcore = LoadLibraryA("Shcore.dll");
	HMODULE hUser32 = LoadLibraryA("user32.dll");
	SetProcessDPIAwarenessFunc setProcDPIAwareness = (SetProcessDPIAwarenessFunc)(hShcore ? GetProcAddress(hShcore, "SetProcessDpiAwareness") : NULL);
	SetProcessDPIAwareFunc setProcDPIAware = (SetProcessDPIAwareFunc)(hUser32 ? GetProcAddress(hUser32, "SetProcessDPIAware") : NULL);

	if (setProcDPIAwareness) /* Windows 8.1+ */
		setProcDPIAwareness(dpi_monitor_aware);
	else if (setProcDPIAware) /* Windows Vista-8.0 */
		setProcDPIAware();

	if (hShcore)
		FreeLibrary(hShcore);
	if (hUser32)
		FreeLibrary(hUser32);
}

//================================================================


/*
================
Sys_Init
================
*/
void Sys_Init (void)
{
	OSVERSIONINFO	vinfo;

	timeBeginPeriod( 1 );

	vinfo.dwOSVersionInfoSize = sizeof(vinfo);

	if (!GetVersionEx (&vinfo))
		Sys_Error ("Couldn't get OS info");

	if (vinfo.dwMajorVersion < 4)
		Sys_Error ("Quake2 requires windows version 4 or greater");
	if (vinfo.dwPlatformId == VER_PLATFORM_WIN32s)
		Sys_Error ("Quake2 doesn't run on Win32s");
	else if ( vinfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS )
		s_win95 = true;

	if (dedicated->value)
	{
		if (!AllocConsole ())
			Sys_Error ("Couldn't create dedicated server console");
		hinput = GetStdHandle (STD_INPUT_HANDLE);
		houtput = GetStdHandle (STD_OUTPUT_HANDLE);
	
		// let QHOST hook in
		InitConProc (argc, argv);
	}
	else if (debug_console->value)
	{
		AllocConsole();
		SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), 0x07);
		DeleteMenu(GetSystemMenu(GetConsoleWindow(), false), SC_CLOSE, MF_BYCOMMAND);
		freopen("CONOUT$", "w", stderr);
	}

	// enable DPI awareness
	Sys_SetDPIAwareness();
}


static char	console_text[256];
static int	console_textlen;

/*
================
Sys_ConsoleInput
================
*/
char *Sys_ConsoleInput (void)
{
	INPUT_RECORD	recs[1024];
	int		dummy;
	int		ch, numread, numevents;

	if (!dedicated || !dedicated->value)
		return NULL;


	for ( ;; )
	{
		if (!GetNumberOfConsoleInputEvents (hinput, &numevents))
			Sys_Error ("Error getting # of console events");

		if (numevents <= 0)
			break;

		if (!ReadConsoleInput(hinput, recs, 1, &numread))
			Sys_Error ("Error reading console input");

		if (numread != 1)
			Sys_Error ("Couldn't read console input");

		if (recs[0].EventType == KEY_EVENT)
		{
			if (!recs[0].Event.KeyEvent.bKeyDown)
			{
				ch = recs[0].Event.KeyEvent.uChar.AsciiChar;

				switch (ch)
				{
					case '\r':
						WriteFile(houtput, "\r\n", 2, &dummy, NULL);	

						if (console_textlen)
						{
							console_text[console_textlen] = 0;
							console_textlen = 0;
							return console_text;
						}
						break;

					case '\b':
						if (console_textlen)
						{
							console_textlen--;
							WriteFile(houtput, "\b \b", 3, &dummy, NULL);	
						}
						break;

					default:
						if (ch >= ' ')
						{
							if (console_textlen < sizeof(console_text)-2)
							{
								WriteFile(houtput, &ch, 1, &dummy, NULL);	
								console_text[console_textlen] = ch;
								console_textlen++;
							}
						}

						break;

				}
			}
		}
	}

	return NULL;
}


/*
================
Sys_ConsoleOutput

Print text to the dedicated console
================
*/
void Sys_ConsoleOutput (char *string)
{
	int		dummy;
	char	text[256];

	if (!dedicated || !dedicated->value)
	{
		if (debug_console && debug_console->value)
		{
			fputs(string, stderr);
			OutputDebugString(string);
		}
		return;
	}

	if (console_textlen)
	{
		text[0] = '\r';
		memset(&text[1], ' ', console_textlen);
		text[console_textlen+1] = '\r';
		text[console_textlen+2] = 0;
		WriteFile(houtput, text, console_textlen+2, &dummy, NULL);
	}

	WriteFile(houtput, string, (DWORD)strlen(string), &dummy, NULL);

	if (console_textlen)
		WriteFile(houtput, console_text, console_textlen, &dummy, NULL);
}


/*
================
Sys_SendKeyEvents

Send Key_Event calls
================
*/
void Sys_SendKeyEvents (void)
{
    MSG        msg;

	while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
	{
		if (!GetMessage (&msg, NULL, 0, 0))
			Sys_Quit ();
		sys_msg_time = msg.time;
      	TranslateMessage (&msg);
      	DispatchMessage (&msg);
	}

	// grab frame time 
	sys_frame_time = timeGetTime();	// FIXME: should this be at start?
}



/*
================
Sys_GetClipboardData

================
*/
char *Sys_GetClipboardData( void )
{
	char *data = NULL;
	char *cliptext;

	if ( OpenClipboard( NULL ) != 0 )
	{
		HANDLE hClipboardData;

		if ( ( hClipboardData = GetClipboardData( CF_TEXT ) ) != 0 )
		{
			if ( ( cliptext = GlobalLock( hClipboardData ) ) != 0 ) 
			{
				data = malloc( GlobalSize( hClipboardData ) + 1 );
				strcpy( data, cliptext );
				GlobalUnlock( hClipboardData );
			}
		}
		CloseClipboard();
	}
	return data;
}

/*
==============================================================================

 WINDOWS CRAP

==============================================================================
*/

/*
=================
Sys_AppActivate
=================
*/
void Sys_AppActivate (void)
{
	ShowWindow ( cl_hwnd, SW_RESTORE);
	SetForegroundWindow ( cl_hwnd );
}

/*
========================================================================

GAME DLL

========================================================================
*/

static HINSTANCE	game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	if (!FreeLibrary (game_library))
		Com_Error (ERR_FATAL, "FreeLibrary failed for game library");
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms)
{
	void	*(*GetGameAPI) (void *);
	char	name[MAX_OSPATH];
	char	*path;
	char	cwd[MAX_OSPATH];
#if defined _M_IX86
	const char *gamename = "gamex86.dll";

#ifdef NDEBUG
	const char *debugdir = "release";
#else
	const char *debugdir = "debug";
#endif

#elif defined _M_X64
	const char *gamename = "gamex64.dll";

#ifdef NDEBUG
	const char *debugdir = "releasex64";
#else
	const char *debugdir = "debugx64";
#endif

#elif defined _M_ALPHA
	const char *gamename = "gameaxp.dll";

#ifdef NDEBUG
	const char *debugdir = "releaseaxp";
#else
	const char *debugdir = "debugaxp";
#endif

#endif

	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	// check the current debug directory first for development purposes
	_getcwd (cwd, sizeof(cwd));
	Com_sprintf (name, sizeof(name), "%s/%s/%s", cwd, debugdir, gamename);
	game_library = LoadLibrary ( name );
	if (game_library)
	{
		Com_DPrintf ("LoadLibrary (%s)\n", name);
	}
	else
	{
#ifdef DEBUG
		// check the current directory for other development purposes
		Com_sprintf (name, sizeof(name), "%s/%s", cwd, gamename);
		game_library = LoadLibrary ( name );
		if (game_library)
		{
			Com_DPrintf ("LoadLibrary (%s)\n", name);
		}
		else
#endif
		{
			// now run through the search paths
			path = NULL;
			while (1)
			{
				path = FS_NextPath (path);
				if (!path)
					return NULL;		// couldn't find one anywhere
				Com_sprintf (name, sizeof(name), "%s/%s", path, gamename);
				game_library = LoadLibrary (name);
				if (game_library)
				{
					Com_DPrintf ("LoadLibrary (%s)\n",name);
					break;
				}
			}
		}
	}

	GetGameAPI = (void *)GetProcAddress (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Sys_UnloadGame ();		
		return NULL;
	}

	return GetGameAPI (parms);
}

//=======================================================================


/*
==================
ParseCommandLine

==================
*/
void ParseCommandLine (LPSTR lpCmdLine)
{
	argc = 1;
	argv[0] = "exe";

	while (*lpCmdLine && (argc < MAX_NUM_ARGVS))
	{
		while (*lpCmdLine && ((*lpCmdLine <= 32) || (*lpCmdLine > 126)))
			lpCmdLine++;

		if (*lpCmdLine)
		{
			argv[argc] = lpCmdLine;
			argc++;

			while (*lpCmdLine && ((*lpCmdLine > 32) && (*lpCmdLine <= 126)))
				lpCmdLine++;

			if (*lpCmdLine)
			{
				*lpCmdLine = 0;
				lpCmdLine++;
			}
			
		}
	}

}

/*
==================
WinMain

==================
*/
HINSTANCE	global_hInstance;

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG				msg;
	int				time, oldtime, newtime;
	char			*cddir;

    /* previous instances do not exist in Win32 */
    if (hPrevInstance)
        return 0;

	global_hInstance = hInstance;

	ParseCommandLine (lpCmdLine);

	// if we find the CD, add a +set cddir xxx command line
	cddir = Sys_ScanForCD ();
	if (cddir && argc < MAX_NUM_ARGVS - 3)
	{
		int		i;

		// don't override a cddir on the command line
		for (i=0 ; i<argc ; i++)
			if (!strcmp(argv[i], "cddir"))
				break;
		if (i == argc)
		{
			argv[argc++] = "+set";
			argv[argc++] = "cddir";
			argv[argc++] = cddir;
		}
	}

	Qcommon_Init (argc, argv);
	oldtime = Sys_Milliseconds ();

    /* main window message loop */
	while (1)
	{
		// if at a full screen console, don't update unless needed
		if (Minimized || (dedicated && dedicated->value) )
		{
			Sleep (1);
		}

		while (PeekMessage (&msg, NULL, 0, 0, PM_NOREMOVE))
		{
			if (!GetMessage (&msg, NULL, 0, 0))
				Com_Quit ();
			sys_msg_time = msg.time;
			TranslateMessage (&msg);
   			DispatchMessage (&msg);
		}

		do
		{
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
		} while (time < 1);

// MSDN: "On the x64 architecture, changing the floating point precision is not supported. 
// If the precision control mask is used on that platform, an assertion and the invalid parameter handler is invoked, 
// as described in Parameter Validation."
#ifdef _M_IX86
		_controlfp( _PC_24, _MCW_PC );
#endif
		if (ActiveApp)
			Qcommon_Frame(time);

		oldtime = newtime;
	}

	// never gets here
    return TRUE;
}
