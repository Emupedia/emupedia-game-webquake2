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

#undef VERSION

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


#ifndef EMSCRIPTEN


static int websocketCallback(struct libwebsocket_context *context, struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
{
	STUBBED("websocketCallback");

	switch (reason) {
	case LWS_CALLBACK_PROTOCOL_INIT:
		Com_Printf("websocketCallback LWS_CALLBACK_PROTOCOL_INIT\n", LOG_NET);
		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY:
		Com_Printf("websocketCallback LWS_CALLBACK_PROTOCOL_DESTROY\n", LOG_NET);
		break;

	case LWS_CALLBACK_GET_THREAD_ID:
		Com_Printf("websocketCallback LWS_CALLBACK_GET_THREAD_ID\n", LOG_NET);
		break;

	case LWS_CALLBACK_ADD_POLL_FD:
		Com_Printf("websocketCallback LWS_CALLBACK_ADD_POLL_FD\n", LOG_NET);
		break;

	case LWS_CALLBACK_DEL_POLL_FD:
		Com_Printf("websocketCallback LWS_CALLBACK_DEL_POLL_FD\n", LOG_NET);
		break;

	case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
		Com_Printf("websocketCallback LWS_CALLBACK_CHANGE_MODE_POLL_FD\n", LOG_NET);
		break;

	case LWS_CALLBACK_LOCK_POLL:
		Com_Printf("websocketCallback LWS_CALLBACK_LOCK_POLL\n", LOG_NET);
		break;

	case LWS_CALLBACK_UNLOCK_POLL:
		break;
		Com_Printf("websocketCallback LWS_CALLBACK_UNLOCK_POLL\n", LOG_NET);

	default:
		Com_Printf ("websocketCallback reason %d\n", LOG_NET, reason);
		break;
	}

	return 0;
}


// not const because libwebsockets writes to it
static struct libwebsocket_protocols protocols[] = {
	  { "quake2", websocketCallback, 0, 0 } /* end */
	, { NULL, NULL, 0, 0 } /* end */
};


static struct libwebsocket_context *websocketContext;


#endif  // EMSCRIPTEN


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
	NET_Common_Init ();
	net_no_recverr = Cvar_Get ("net_no_recverr", "0", 0);
}


int NET_IPSocket (char *net_interface, int port)
{
#ifdef EMSCRIPTEN

	STUBBED("NET_IPSocket");
	// TODO: on emscripten pre-establish server connection
	return 0;

#else  // EMSCRIPTEN

	struct lws_context_creation_info info;
	memset(&info, 0, sizeof(info));
	// TODO: CONTEXT_PORT_NO_LISTEN when NO_SERVER?
	info.port = port;
	info.protocols = protocols;
	info.gid = -1;
	info.uid = -1;
	// TODO: SSL?
	// TODO: put keepalive stuff in cvars
	info.ka_time = 5;
	info.ka_probes = 3;
	info.ka_interval = 1;

	websocketContext = libwebsocket_create_context(&info);
	if (!websocketContext) {
		return 0;
	}

	int retval = libwebsocket_service(websocketContext, 0);
	Com_Printf("libwebsocket_service returned %d\n", LOG_NET, retval);

#endif  // EMSCRIPTEN

	return 1;
}


int NET_SendPacket (netsrc_t sock, int length, const void *data, netadr_t *to)
{
	STUBBED("NET_SendPacket");
	return 0;
}

#endif  // defined(USE_LIBWEBSOCKETS) || defined(EMSCRIPTEN)
