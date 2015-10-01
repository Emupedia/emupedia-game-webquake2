#if defined(USE_LIBWEBSOCKETS) || defined(EMSCRIPTEN)


#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif  // WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>


#else  // _WIN32

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#endif  // _WIN32


#ifndef EMSCRIPTEN
#include <libwebsockets.h>
#endif  // EMSCRIPTEN


extern "C" {


#include "../qcommon/qcommon.h"


static unsigned int net_inittime;

static unsigned long long net_total_in;
static unsigned long long net_total_out;
static unsigned long long net_packets_in;
static unsigned long long net_packets_out;

int			server_port;
//netadr_t	net_local_adr;

static int			ip_sockets[2];

char *NET_ErrorString (void);

cvar_t	*net_no_recverr;

//Aiee...
#include "../qcommon/net_common.c"


}


#ifdef NDEBUG


#define STUBBED(x)


#else  // NDEBUG


#define STUBBED(x) do { \
	static bool seen_this = false; \
	if (!seen_this) { \
		seen_this = true; \
		Com_Printf ("STUBBED: %s at %s (%s:%d)\n", LOG_NET, \
		x, __FUNCTION__, __FILE__, __LINE__); \
	} \
	} while (0)


#endif  // NDEBUG


void Net_Stats_f (void)
{
	int now = time(0);
	int diff = now - net_inittime;

	Com_Printf ("Network up for %i seconds.\n"
				"%llu bytes in %llu packets received (av: %i kbps)\n"
				"%llu bytes in %llu packets sent (av: %i kbps)\n", LOG_NET,
				
				diff,
				net_total_in, net_packets_in, (int)(((net_total_in * 8) / 1024) / diff),
				net_total_out, net_packets_out, (int)((net_total_out * 8) / 1024) / diff);
}


int	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	if (NET_GetLoopPacket (sock, net_from, net_message))
		return 1;

	STUBBED("NET_GetPacket");

	return 0;
}


void NET_Init (void)
{
	// TODO: init libwebsockets
	// TODO: on emscripten pre-establish server connection
	STUBBED("NET_Init");
}


int NET_IPSocket (char *net_interface, int port)
{
	STUBBED("NET_IPSocket");
	return 0;
}


int NET_SendPacket (netsrc_t sock, int length, const void *data, netadr_t *to)
{
	STUBBED("NET_SendPacket");
	return 0;
}

#endif  // defined(USE_LIBWEBSOCKETS) || defined(EMSCRIPTEN)
