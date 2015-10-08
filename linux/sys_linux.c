#ifndef _WIN32

#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <sys/utsname.h>
#define __USE_GNU 1

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif  // _GNU_SOURCE

#include <link.h>
#include <sys/ucontext.h>
#include <sys/resource.h>

//for old headers
#ifndef REG_EIP
#ifndef EIP
#define EIP 12 //aiee
#endif
#define REG_EIP EIP
#endif

//#include <fenv.h>
#include <dlfcn.h>

#ifndef EMSCRIPTEN

#include <sys/wait.h>
#include <execinfo.h>

#else  // EMSCRIPTEN

#include <emscripten.h>

#endif  // EMSCRIPTEN

#include "../qcommon/qcommon.h"

#include "../client/ref.h"

#include "../game/game.h"


cvar_t *nostdin;
cvar_t *nostdout;

extern cvar_t *sys_loopstyle;

unsigned	sys_frame_time;

//uid_t saved_euid;
qboolean stdin_active = true;

// =======================================================================
// General routines
// =======================================================================
#ifndef NO_SERVER
void Sys_ConsoleOutput (const char *string)
{
	char	text[2048];
	int		i, j;

	if (nostdout && nostdout->intvalue)
		return;

	i = 0;
	j = 0;

	//strip high bits
	while (string[j])
	{
		text[i] = string[j] & 127;

		//strip low bits
		if (text[i] >= 32 || text[i] == '\n' || text[i] == '\t')
			i++;

		j++;

		if (i == sizeof(text)-2)
		{
			text[i++] = '\n';
			break;
		}
	}
	text[i] = 0;

	fputs(text, stdout);
}
#endif

int Sys_FileLength (const char *path)
{
	struct stat st;

	if (stat (path, &st) || (st.st_mode & S_IFDIR))
		return -1;

	return st.st_size;
}

void Sys_getrusage_f (void)
{
	struct rusage usage;

	getrusage (RUSAGE_SELF, &usage);

	//FIXME
	Com_Printf ("user:", LOG_GENERAL);
}

void Sys_Sleep (int msec)
{
	usleep (msec*1000);
}

void Sys_SetWindowText (char *dummy)
{
}


void Sys_Quit (void)
{
	CL_Shutdown ();
	Qcommon_Shutdown ();

#ifdef EMSCRIPTEN

	emscripten_cancel_main_loop();

#else  // EMSCRIPTEN

	fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);

#endif  // EMSCRIPTEN

	exit(0);
}

void Sys_KillServer (int sig)
{
	signal (SIGTERM, SIG_DFL);
	signal (SIGINT, SIG_DFL);

	Com_Printf ("Got sig %d, shutting down.\n", LOG_SERVER|LOG_NOTICE, sig);
	Cmd_TokenizeString (va("Exiting on signal %d\n", sig), 0);
	Com_Quit();
}

#if R1RELEASE == 3
static int dlcallback (struct dl_phdr_info *info, size_t size, void *data)
{
	int		j;
	int		end;
	
	end = 0;

	if (!info->dlpi_name || !info->dlpi_name[0])
		return 0;
	
	for (j = 0; j < info->dlpi_phnum; j++)
	{
		end += info->dlpi_phdr[j].p_memsz;
	}

	//this is terrible.
#if __WORDSIZE == 64
	fprintf (stderr, "[0x%lux-0x%lux] %s\n", info->dlpi_addr, info->dlpi_addr + end, info->dlpi_name);
#else
	fprintf (stderr, "[0x%ux-0x%ux] %s\n", info->dlpi_addr, info->dlpi_addr + end, info->dlpi_name);
#endif
	return 0;
}
#endif


#ifndef EMSCRIPTEN

/* Obtain a backtrace and print it to stderr. 
 * Adapted from http://www.delorie.com/gnu/docs/glibc/libc_665.html
 */
#ifdef __x86_64__
void Sys_Backtrace (int sig)
#else
void Sys_Backtrace (int sig, siginfo_t *siginfo, void *secret)
#endif
{
	void		*array[32];
	struct utsname	info;
	size_t		size;
	size_t		i;
	char		**strings;
#ifndef __x86_64__
	ucontext_t 	*uc = (ucontext_t *)secret;
#endif

	signal (SIGSEGV, SIG_DFL);
	
	fprintf (stderr, "=====================================================\n"
			 "Segmentation Fault\n"
			 "=====================================================\n"
			 "A crash has occured within R1Q2 or the Game DLL (mod)\n"
			 "that you are running.  This is most  likely caused by\n"
			 "using the wrong server binary (eg, r1q2ded instead of\n"
			 "r1q2ded-old) for the mod you are running.  The server\n"
			 "README on the  R1Q2 forums has more information about\n"
			 "which binaries you should be using.\n"
			 "\n"
			 "If possible, try re-building R1Q2 and the mod you are\n"
			 "running from source code to ensure it isn't a compile\n"
			 "problem. If the crash still persists, please post the\n"
			 "following  debug info on the R1Q2 forums with details\n"
			 "including the mod name,  version,  Linux distribution\n"
			 "and any other pertinent information.\n"
			 "\n");

	size = backtrace (array, sizeof(array)/sizeof(void*));

#ifndef __x86_64__
	array[1] = (void *) uc->uc_mcontext.gregs[REG_EIP];
#endif
	
	strings = backtrace_symbols (array, size);

	fprintf (stderr, "Stack dump (%zd frames):\n", size);

	for (i = 0; i < size; i++)
		fprintf (stderr, "%.2zd: %s\n", i, strings[i]);

	fprintf (stderr, "\nVersion: " R1BINARY " " VERSION " (" BUILDSTRING " " CPUSTRING ") " RELEASESTRING "\n");

	uname (&info);
	fprintf (stderr, "OS Info: %s %s %s %s %s\n\n", info.sysname, info.nodename, info.release, info.version, info.machine);

#if R1RELEASE == 3
	fprintf (stderr, "Loaded libraries:\n");
	dl_iterate_phdr(dlcallback, NULL);
#endif
	
	free (strings);

	raise (SIGSEGV);
}


#endif  // EMSCRIPTEN


void Sys_ProcessTimes_f (void)
{
	struct rusage	usage;
	int		days, hours, minutes;
	double		seconds;
	
	if (getrusage (RUSAGE_SELF, &usage))
	{
		Com_Printf ("getrusage(): %s\n", LOG_GENERAL, strerror(errno));
		return;
	}

	//1 second = 1 000 000 microseconds
	days = usage.ru_stime.tv_sec / 86400;
	usage.ru_stime.tv_sec -= days * 86400;

	hours = usage.ru_stime.tv_sec / 3600;
	usage.ru_stime.tv_sec -= hours * 3600;

	minutes = usage.ru_stime.tv_sec / 60;
	usage.ru_stime.tv_sec -= minutes * 60;

	seconds = usage.ru_stime.tv_sec + (usage.ru_stime.tv_usec / 1000000);
	
	Com_Printf ("%dd %dh %dm %gs kernel\n", LOG_GENERAL, days, hours, minutes, seconds);

	days = usage.ru_utime.tv_sec / 86400;
	usage.ru_utime.tv_sec -= days * 86400;

	hours = usage.ru_utime.tv_sec / 3600;
	usage.ru_utime.tv_sec -= hours * 3600;

	minutes = usage.ru_utime.tv_sec / 60;
	usage.ru_utime.tv_sec -= minutes * 60;

	seconds = usage.ru_utime.tv_sec + (usage.ru_utime.tv_usec / 1000000);

	Com_Printf ("%dd %dh %dm %gs user\n", LOG_GENERAL, days, hours, minutes, seconds);
}

static unsigned int goodspins, badspins;

void Sys_Spinstats_f (void)
{
	Com_Printf ("%u fast spins, %u slow spins, %.2f%% slow.\n", LOG_GENERAL, goodspins, badspins, ((float)badspins / (float)(goodspins+badspins)) * 100.0f);
}

unsigned short Sys_GetFPUStatus (void)
{
#ifndef EMSCRIPTEN
	unsigned short fpuword;
	__asm__ __volatile__ ("fnstcw %0" : "=m" (fpuword));
	return fpuword;

#else  // EMSCRIPTEN

	return 0;
#endif  // EMSCRIPTEN
}

/*
 * Round to zero, 24 bit precision
 */
void Sys_SetFPU (void)
{
#ifndef EMSCRIPTEN
	unsigned short fpuword;
	fpuword = Sys_GetFPUStatus ();
	fpuword &= ~(3 << 8);
	fpuword |= (0 << 8);
	fpuword &= ~(3 << 10);
	fpuword |= (0 << 10);
	__asm__ __volatile__ ("fldcw %0" : : "m" (fpuword));
#endif  // EMSCRIPTEN
}

void Sys_Init(void)
{
#if id386
//	Sys_SetFPCW();
#endif
  /* Install our signal handler */

#if !defined(EMSCRIPTEN) && !defined(USE_AFL)

#ifndef __x86_64__
	struct sigaction sa;

	if (sizeof(uint32) != 4)
		Sys_Error ("uint32 != 32 bits");
	else if (sizeof(uint64) != 8)
		Sys_Error ("uint64 != 64 bits");
	else if (sizeof(uint16) != 2)
		Sys_Error ("uint16 != 16 bits");

//	fesetround (FE_TOWARDZERO);

	Sys_SetFPU ();
	Sys_CheckFPUStatus ();

	sa.sa_sigaction = (void *)Sys_Backtrace;
	sigemptyset (&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGSEGV, &sa, NULL);
#else
	signal (SIGSEGV, Sys_Backtrace);
#endif
	
	signal (SIGTERM, Sys_KillServer);
	signal (SIGINT, Sys_KillServer);

#endif  // EMSCRIPTEN

	//initialize timer base
	Sys_Milliseconds ();
}

void Sys_Error (const char *error, ...)
{ 
    va_list     argptr;
    char        string[1024];

#ifndef EMSCRIPTEN
// change stdin to non blocking
    fcntl (0, F_SETFL, fcntl (0, F_GETFL, 0) & ~FNDELAY);
#endif  // EMSCRIPTEN

	CL_Shutdown ();
	Qcommon_Shutdown ();
    
    va_start (argptr,error);
    vsprintf (string,error,argptr);
    va_end (argptr);
	fprintf(stderr, "Error: %s\n", string);

	exit (1);

} 


/*
============
Sys_FileTime

returns -1 if not present
============
*/
int	Sys_FileTime (char *path)
{
	struct	stat	buf;
	
	if (stat (path,&buf) == -1)
		return -1;
	
	return buf.st_mtime;
}

void floating_point_exception_handler(int whatever)
{
	signal(SIGFPE, floating_point_exception_handler);
}


#ifndef NO_SERVER


char *Sys_ConsoleInput(void)
{
	if (!dedicated || !dedicated->intvalue)
		return NULL;

	if (!stdin_active || (nostdin && nostdin->intvalue))
		return NULL;

	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(0, &fdset); // stdin
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	if (select (1, &fdset, NULL, NULL, &timeout) < 1 || !FD_ISSET(0, &fdset))
		return NULL;

	static char text[1024];
	int len = read (0, text, sizeof(text));
	if (len == 0)
	{ // eof!
		//stdin_active = false;
		return NULL;
	}
	else if (len == sizeof(text))
	{
		Com_Printf ("Sys_ConsoleInput: Line too long, discarded.\n", LOG_SERVER);
		return NULL;
	}

	if (len < 1)
		return NULL;

	text[len-1] = 0;    // rip off the /n and terminate

	return text;
}


#endif  // NO_SERVER


/*****************************************************************************/

static void *game_library;

/*
=================
Sys_UnloadGame
=================
*/
void Sys_UnloadGame (void)
{
	if (game_library) 
		dlclose (game_library);
	game_library = NULL;
}

/*
=================
Sys_GetGameAPI

Loads the game dll
=================
*/
void *Sys_GetGameAPI (void *parms, int baseq2)
{
#if defined(GAME_HARD_LINKED)

	return GetGameAPI(parms);

#else  // BUILTIN_GAME

	void	*(*GetGameAPI) (void *);

	char	name[MAX_OSPATH];
	char	curpath[MAX_OSPATH];
	char	*path;
#ifdef __i386__
	const char *gamename = "gamei386.so";
#elif defined __alpha__
	const char *gamename = "gameaxp.so";
#elif defined __x86_64__
	const char *gamename = "gamex86_64.so";
#else
#error "Don't know what kind of dynamic objects to use for this architecture."
#endif

	if (game_library)
		Com_Error (ERR_FATAL, "Sys_GetGameAPI without Sys_UnloadingGame");

	getcwd(curpath, sizeof(curpath)-1);
	curpath[sizeof(curpath)-1] = 0;

	Com_Printf("------- Loading %s -------\n", LOG_SERVER|LOG_NOTICE, gamename);

	if (baseq2)
	{
		Com_sprintf (name, sizeof(name), "%s/%s/%s", curpath, BASEDIRNAME, gamename);
		game_library = dlopen (name, RTLD_NOW );

		if (game_library == NULL) {
			Com_Printf ("dlopen(): %s\n", LOG_SERVER, dlerror());
			Com_Printf ("Attempting to load with lazy symbols...", LOG_SERVER);
			game_library = dlopen(name, RTLD_LAZY);
		}
	}
	else
	{
		// now run through the search paths
		path = NULL;
		for (;;)
		{
			path = FS_NextPath (path);
			if (!path)
				return NULL;		// couldn't find one anywhere
			Com_sprintf (name, sizeof(name), "%s/%s/%s", curpath, path, gamename);
			game_library = dlopen (name, RTLD_NOW );
			if (game_library)
			{
				Com_DPrintf ("LoadLibrary (%s)\n",name);
				break;
			}
			else
			{
				Com_Printf ("dlopen(): %s\n", LOG_SERVER, dlerror());
				Com_Printf ("Attempting to load with lazy symbols...", LOG_SERVER);
				game_library = dlopen(name, RTLD_LAZY);
				if (game_library)
				{
					Com_Printf ("ok\n", LOG_SERVER);
					break;
				}
				else
				{
					Com_Printf ("dlopen(): %s\n", LOG_SERVER, dlerror());
				}
			}
		}
	}

	if (!game_library)
		return NULL;

	GetGameAPI = (void *(*)(void *))dlsym (game_library, "GetGameAPI");
	if (!GetGameAPI)
	{
		Com_Printf ("dlsym(): %s\n", LOG_SERVER, dlerror());
		Sys_UnloadGame ();		
		return NULL;
	}

	return GetGameAPI (parms);
#endif  // BUILTIN_GAME
}

/*****************************************************************************/

void Sys_AppActivate (void)
{
}

void Sys_SendKeyEvents (void)
{
	KBD_Update();

	// grab frame time 
	sys_frame_time = Sys_Milliseconds();
}

/*****************************************************************************/

char *Sys_GetClipboardData(void)
{
	return NULL;
}


#ifdef EMSCRIPTEN


static unsigned int g_oldtime = 0;

void mainloop() {
	unsigned int newtime = Sys_Milliseconds();
	unsigned int time = newtime - g_oldtime;

	if (time >= 1) {
		g_oldtime = newtime;
		Qcommon_Frame(time);
	}
}


#endif  // EMSCRIPTEN


int main (int argc, char **argv)
{
	unsigned int 	time, oldtime, newtime, spins;

	// go back to real user for config loads
	//saved_euid = geteuid();
	//seteuid(getuid());
	//

#ifndef EMSCRIPTEN
	if (getuid() == 0 || geteuid() == 0)
		Sys_Error ("For security reasons, do not run Quake II as root.");
#endif  // EMSCRIPTEN

	binary_name = argv[0];

	Qcommon_Init(argc, argv);

#ifndef EMSCRIPTEN
	fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
#endif  // EMSCRIPTEN

	nostdin = Cvar_Get ("nostdin", "0", 0);
	nostdout = Cvar_Get("nostdout", "0", 0);
#ifndef EMSCRIPTEN
	if (!nostdout->intvalue) {
		fcntl(0, F_SETFL, fcntl (0, F_GETFL, 0) | FNDELAY);
//		printf ("Linux Quake -- Version %0.3f\n", LINUX_VERSION);
	}
#endif  // EMSCRIPTEN

#ifdef EMSCRIPTEN

	char *addrStr = getenv("Q2SERVER");
	if (addrStr != NULL) {
	netadr_t adr;
	NET_StringToAdr(addrStr, &adr);

		NET_PreConnect(&adr);
	}

	g_oldtime = Sys_Milliseconds();
	emscripten_set_main_loop(mainloop, 0, 1);

#else  // EMSCRIPTEN
	oldtime = Sys_Milliseconds ();
    while (1)
    {
		// find time spent rendering last frame
#ifndef NO_SERVER
		if (dedicated->intvalue && sys_loopstyle->intvalue)
		{
			newtime = Sys_Milliseconds ();
			time = newtime - oldtime;
			spins = 0;
		}
		else
#endif  // NO_SERVER
		{
			spins = 0;
			do
			{
				newtime = Sys_Milliseconds ();
				time = newtime - oldtime;
				spins++;
			} while (time < 1);
		}

		if (spins > 500)
			badspins++;
		else
			goodspins++;
		
		Qcommon_Frame (time);
		oldtime = newtime;
    }
#endif  // EMSCRIPTEN
}

//r1 :redundant
void Sys_CopyProtect(void)
{
}


qboolean Sys_CheckFPUStatus (void)
{
	static unsigned short	last_word = 0;
	unsigned short	fpu_control_word;

	fpu_control_word = Sys_GetFPUStatus ();

	Com_DPrintf ("Sys_CheckFPUStatus: rounding %d, precision %d\n", (fpu_control_word >> 10) & 3, (fpu_control_word >> 8) & 3);

	//check rounding (10) and precision (8) are set properly
	if (((fpu_control_word >> 10) & 3) != 0 ||
		((fpu_control_word >> 8) & 3) != 0)
	{
		if (fpu_control_word != last_word)
		{
			last_word = fpu_control_word;
			return false;
		}
	}

	last_word = fpu_control_word;
	return true;
}

void Sys_ShellExec (const char *cmd)
{
	//FIXME
}

void Sys_OpenURL (void)
{
	//FIXME
}

void Sys_UpdateURLMenu (const char *s)
{
	//FIXME
}


#endif  // !_WIN32
