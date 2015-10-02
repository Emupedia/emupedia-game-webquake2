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


static unsigned int net_inittime = 0;

static unsigned long long net_total_in = 0;
static unsigned long long net_total_out = 0;
static unsigned long long net_packets_in = 0;
static unsigned long long net_packets_out = 0;

int			server_port = 0;
//netadr_t	net_local_adr;

static int			ip_sockets[2] = { 0, 0 };

cvar_t	*net_no_recverr = NULL;


char *NET_ErrorString (void);
int NET_IPSocket (char *net_interface, int port);
void NET_OpenIP (int flags);
void Net_Restart_f (void);
void Net_Stats_f (void);
qboolean	NET_StringToSockaddr (const char *s, struct sockaddr *sadr);


/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
/*#define DO(src,dest)	\
	copy[0] = s[src];	\
	copy[1] = s[src + 1];	\
	sscanf (copy, "%x", &val);	\
	((struct sockaddr_ipx *)sadr)->dest = val*/


#ifndef _WIN32
#define closesocket close
#define ioctlsocket ioctl
#define SOCKET unsigned int
#define INVALID_SOCKET -1
#endif

int _true = 1;

static	cvar_t	*net_ignore_icmp;


netadr_t	net_proxy_addr;
qboolean	net_proxy_active;

void NetadrToSockadr (netadr_t *a, struct sockaddr_in *s)
{
	memset (s, 0, sizeof(*s));

	if (a->type == NA_IP)
	{
		s->sin_family = AF_INET;

		*(int *)&s->sin_addr = *(int *)&a->ip;
		s->sin_port = a->port;
	}
	else if (a->type == NA_BROADCAST)
	{
		s->sin_family = AF_INET;

		s->sin_port = a->port;
		*(int *)&s->sin_addr = -1;
	}
}


char	*NET_AdrToString (netadr_t *a)
{
	static	char	s[32];
	
	Com_sprintf (s, sizeof(s), "%i.%i.%i.%i:%i", a->ip[0], a->ip[1], a->ip[2], a->ip[3], ntohs(a->port));

	return s;
}


char	*NET_BaseAdrToString (netadr_t *a)
{
	static	char	s[32];
	
	Com_sprintf (s, sizeof(s), "%i.%i.%i.%i", a->ip[0], a->ip[1], a->ip[2], a->ip[3]);

	return s;
}


void NET_Common_Init (void)
{
	net_ignore_icmp = Cvar_Get ("net_ignore_icmp", "0", 0);

	Cmd_AddCommand ("net_restart", Net_Restart_f);
	Cmd_AddCommand ("net_stats", Net_Stats_f);
}


uint32 NET_htonl (uint32 ip)
{
	return htonl (ip);
}


char	*NET_inet_ntoa (uint32 ip)
{
	return inet_ntoa (*(struct in_addr *)&ip);
}


uint32 NET_ntohl (uint32 ip)
{
	return ntohl (ip);
}


#define SockadrToNetadr(s,a) \
	a->type = NA_IP; \
	*(int *)&a->ip = ((struct sockaddr_in *)s)->sin_addr.s_addr; \
	a->port = ((struct sockaddr_in *)s)->sin_port; \


/*
=============
NET_StringToAdr

localhost
idnewt
idnewt:28000
192.246.40.70
192.246.40.70:28000
=============
*/
qboolean	NET_StringToAdr (const char *s, netadr_t *a)
{
	struct sockaddr sadr;

	if (!strcmp (s, "localhost"))
	{
		memset (a, 0, sizeof(*a));

		//r1: should need some kind of ip data to prevent comparisons with empty ips?
		a->ip[0] = 127;
		a->ip[3] = 1;
		a->port = PORT_SERVER;
		a->type = NA_LOOPBACK;
		return true;
	}

	if (!NET_StringToSockaddr (s, &sadr))
		return false;
	
	SockadrToNetadr (&sadr, a);

	return true;
}


qboolean	NET_StringToSockaddr (const char *s, struct sockaddr *sadr)
{
	int	isip = 0;
	const char *p;
	struct hostent	*h;
	char	*colon;
//	int		val;
	char	copy[128];
	
	memset (sadr, 0, sizeof(*sadr));

	//r1: better than just the first digit for ip validity :)
	p = s;
	while (p[0])
	{
		if (p[0] == '.')
		{
			isip++;
		}
		else if (p[0] == ':') 
		{
			break;
		}
		else if (!isdigit(p[0]))
		{
			isip = -1;
			break;
		}
		p++;
	}

	if (isip != -1 && isip != 3)
		return false;
		
	((struct sockaddr_in *)sadr)->sin_family = AF_INET;
	
	((struct sockaddr_in *)sadr)->sin_port = 0;

	//r1: CHECK THE GODDAMN BUFFER SIZE... sigh yet another overflow.
	Q_strncpy (copy, s, sizeof(copy)-1);

	// strip off a trailing :port if present
	for (colon = copy ; colon[0] ; colon++)
	{
		if (colon[0] == ':')
		{
			colon[0] = 0;
			((struct sockaddr_in *)sadr)->sin_port = htons ((int16)atoi(colon+1));
			break;
		}
	}
	
	if (isip != -1)
	{
		((struct sockaddr_in *)sadr)->sin_addr.s_addr = inet_addr(copy);
	}
	else
	{
		if (! (h = gethostbyname(copy)) )
			return false;
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
	}
	
	return true;
}


/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
int	NET_Config (int toOpen)
{
	int		i;
	static	int	old_config;

	i = old_config;

	if (old_config == toOpen)
		return i;

	old_config |= toOpen;

	if (toOpen == NET_NONE)
	{
		if (ip_sockets[NS_CLIENT])
		{
			closesocket (ip_sockets[NS_CLIENT]);
			ip_sockets[NS_CLIENT] = 0;
		}

		if (ip_sockets[NS_SERVER])
		{
			closesocket (ip_sockets[NS_SERVER]);
			ip_sockets[NS_SERVER] = 0;
		}

		old_config = NET_NONE;
	}

	NET_OpenIP (toOpen);

	return i;
}

void NET_SetProxy (netadr_t *proxy)
{
	if (proxy)
	{
		net_proxy_addr = *proxy;
		net_proxy_active = true;
	}
	else
		net_proxy_active = false;
}


#define	MAX_LOOPBACK	4

typedef struct
{
	byte	data[MAX_MSGLEN];
	int		datalen;
} loopmsg_t;

typedef struct
{
	loopmsg_t	msgs[MAX_LOOPBACK];
	int			get, send;
} loopback_t;

loopback_t	loopbacks[2];


int NET_Client_Sleep (int msec)
{
    struct timeval	timeout;
	fd_set			fdset;
	SOCKET			i;

	FD_ZERO(&fdset);
	i = 0;

	if (ip_sockets[NS_CLIENT])
	{
		FD_SET(ip_sockets[NS_CLIENT], &fdset); // network socket
		i = ip_sockets[NS_CLIENT];
	}

	timeout.tv_sec = msec/1000;
	timeout.tv_usec = (msec%1000)*1000;
	return select ((int)(i+1), &fdset, NULL, NULL, &timeout);
}


qboolean	NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return false;

	i = loop->get & (MAX_LOOPBACK-1);
	loop->get++;

	memcpy (net_message->data, loop->msgs[i].data, loop->msgs[i].datalen);
	net_message->cursize = loop->msgs[i].datalen;
	memset (net_from, 0, sizeof(*net_from));
	net_from->type = NA_LOOPBACK;
	net_from->ip[0] = 127;
	net_from->ip[3] = 1;
	net_from->port = PORT_SERVER;
	return true;

}


/*
====================
NET_OpenIP
====================
*/
void NET_OpenIP (int flags)
{
	cvar_t	*ip;
	int		port;
	int		dedicated;

	net_total_in = net_packets_in = net_total_out = net_packets_out = 0;
	net_inittime = (unsigned int)time(NULL);

	ip = Cvar_Get ("ip", "localhost", CVAR_NOSET);

	dedicated = Cvar_IntValue ("dedicated");

	if (flags & NET_SERVER)
	{
		if (!ip_sockets[NS_SERVER])
		{
			port = Cvar_Get("ip_hostport", "0", CVAR_NOSET)->intvalue;
			if (!port)
			{
				port = Cvar_Get("hostport", "0", CVAR_NOSET)->intvalue;
				if (!port)
				{
					port = Cvar_Get("port", va("%i", PORT_SERVER), CVAR_NOSET)->intvalue;
				}
			}
			server_port = port;
			ip_sockets[NS_SERVER] = NET_IPSocket (ip->string, port);
			if (!ip_sockets[NS_SERVER] && dedicated)
				Com_Error (ERR_FATAL, "Couldn't allocate dedicated server IP port on %s:%d. Another application is probably using it.", ip->string, port);
		}
	}

	// dedicated servers don't need client ports
	if (dedicated)
		return;

	if (!ip_sockets[NS_CLIENT])
	{
		int newport = (int)(random() * 64000 + 1024);
		port = Cvar_Get("ip_clientport", va("%i", newport), CVAR_NOSET)->intvalue;
		if (!port)
		{
			
			port = Cvar_Get("clientport", va("%i", newport) , CVAR_NOSET)->intvalue;
			if (!port) {
				port = PORT_ANY;
				Cvar_Set ("clientport", va ("%d", newport));
			}
		}

		ip_sockets[NS_CLIENT] = NET_IPSocket (ip->string, newport);
		if (!ip_sockets[NS_CLIENT])
			ip_sockets[NS_CLIENT] = NET_IPSocket (ip->string, PORT_ANY);
	}

	if (!ip_sockets[NS_CLIENT])
		Com_Error (ERR_DROP, "Couldn't allocate client IP port.");
}


void Net_Restart_f (void)
{
	int old;
	old = NET_Config (NET_NONE);
	NET_Config (old);
}


void NET_SendLoopPacket (netsrc_t sock, int length, const void *data)
{
	int		i;
	loopback_t	*loop;

	loop = &loopbacks[sock^1];

	i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}


// sleeps msec or until net socket is ready
#ifndef NO_SERVER
void NET_Sleep(int msec)
{
    struct timeval timeout;
	fd_set	fdset;
	extern cvar_t *dedicated;
	//extern qboolean stdin_active;

	if (!ip_sockets[NS_SERVER] || !dedicated->intvalue)
		return; // we're not a server, just run full speed

	//Com_Printf ("NET_Sleep (%d)\n", LOG_GENERAL, msec);

	FD_ZERO(&fdset);
	FD_SET(ip_sockets[NS_SERVER], &fdset); // network socket
	timeout.tv_sec = msec/1000;
	timeout.tv_usec = (msec%1000)*1000;
	select ((int)(ip_sockets[NS_SERVER]+1), &fdset, NULL, NULL, &timeout);
}
#endif


}  // extern "C"


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
