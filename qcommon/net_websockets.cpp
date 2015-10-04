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


}  // extern "C"


#include <memory>
#include <tuple>
#include <unordered_map>


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


static unsigned int net_inittime = 0;

static unsigned long long net_total_in = 0;
static unsigned long long net_total_out = 0;
static unsigned long long net_packets_in = 0;
static unsigned long long net_packets_out = 0;

int			server_port = 0;
//netadr_t	net_local_adr;

cvar_t	*net_no_recverr = NULL;


int _true = 1;

static	cvar_t	*net_ignore_icmp = NULL;


netadr_t	net_proxy_addr = { NA_IP, { 0, 0, 0, 0 }, 0 };
qboolean	net_proxy_active = false;


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


static bool websocketInitialized = false;


struct Connection;


namespace std {


template <> struct hash<netadr_t> {
	typedef netadr_t argument_type;
	typedef std::size_t result_type;

	result_type operator()(const argument_type &val) const {
		uint32_t temp = val.port;
		temp = (temp << 16) | val.type;

		result_type h1 = std::hash<uint32_t>()(temp);

		temp =                val.ip[0];
		temp = (temp << 8 ) | val.ip[1];
		temp = (temp << 8 ) | val.ip[2];
		temp = (temp << 8 ) | val.ip[3];

		result_type h2 = std::hash<uint32_t>()(temp);

		return h1 ^ (h2 << 2);
	}
};


}  // namespace std


bool operator==(const netadr_t &a, const netadr_t &b) {
	if (a.type != b.type) {
		return false;
	}

	return NET_CompareAdr(&a, &b);
}


static std::unordered_map<netadr_t, std::unique_ptr<Connection> > connections;


#ifndef EMSCRIPTEN


struct Connection {
	netadr_t addr;
	struct libwebsocket *wsi;


	Connection(netadr_t addr_, struct libwebsocket *wsi_)
	: addr(addr_)
	, wsi(wsi_)
	{
	}


	~Connection()
	{
		STUBBED("~Connection");
	}
};


static int websocketCallback(struct libwebsocket_context *context, struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason, void *user, void *in, size_t len)
{
	switch (reason) {
	case LWS_CALLBACK_ESTABLISHED:
		Com_Printf("websocketCallback LWS_CALLBACK_ESTABLISHED\n", LOG_NET);
		break;

	case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
		Com_Printf("websocketCallback LWS_CALLBACK_CLIENT_CONNECTION_ERROR\n", LOG_NET);
		break;

	case LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH:
		Com_Printf("websocketCallback LWS_CALLBACK_CLIENT_FILTER_PRE_ESTABLISH\n", LOG_NET);
		break;

	case LWS_CALLBACK_CLIENT_ESTABLISHED:
		Com_Printf("websocketCallback LWS_CALLBACK_CLIENT_ESTABLISHED\n", LOG_NET);
		break;

	case LWS_CALLBACK_CLOSED:
		Com_Printf("websocketCallback LWS_CALLBACK_CLOSED\n", LOG_NET);
		break;

	case LWS_CALLBACK_FILTER_NETWORK_CONNECTION:
		Com_Printf("websocketCallback LWS_CALLBACK_FILTER_NETWORK_CONNECTION\n", LOG_NET);
		break;

	case LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED:
		Com_Printf("websocketCallback LWS_CALLBACK_SERVER_NEW_CLIENT_INSTANTIATED\n", LOG_NET);
		break;

	case LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION:
		Com_Printf("websocketCallback LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION\n", LOG_NET);
		break;

	case LWS_CALLBACK_PROTOCOL_INIT:
		// ignored
		break;

	case LWS_CALLBACK_PROTOCOL_DESTROY:
		// ignored
		break;

	case LWS_CALLBACK_WSI_CREATE:
		Com_Printf("websocketCallback LWS_CALLBACK_WSI_CREATE\n", LOG_NET);
		break;

	case LWS_CALLBACK_WSI_DESTROY:
		Com_Printf("websocketCallback LWS_CALLBACK_WSI_DESTROY\n", LOG_NET);
		break;

	case LWS_CALLBACK_GET_THREAD_ID:
		// ignored
		break;

	case LWS_CALLBACK_ADD_POLL_FD:
		// ignored
		break;

	case LWS_CALLBACK_DEL_POLL_FD:
		// ignored
		break;

	case LWS_CALLBACK_CHANGE_MODE_POLL_FD:
		// ignored
		break;

	case LWS_CALLBACK_LOCK_POLL:
		// ignored
		break;

	case LWS_CALLBACK_UNLOCK_POLL:
		// ignored
		break;

	default:
		Com_Printf ("websocketCallback reason %d\n", LOG_NET, reason);
		break;
	}

	return 0;
}


// not const because libwebsockets writes to it
static struct libwebsocket_protocols protocols[] = {
	  { "quake2", websocketCallback, 0, 0, 0, NULL, NULL, 0 }
	, { NULL    , NULL             , 0, 0, 0, NULL, NULL, 0 } /* end */
};


static struct libwebsocket_context *websocketContext = NULL;


static bool createWebsocketContext(int port) {
	assert(!websocketInitialized);
	assert(!websocketContext);

	struct lws_context_creation_info info;
	memset(&info, 0, sizeof(info));
	if (port == PORT_ANY) {
		info.port = CONTEXT_PORT_NO_LISTEN;
	} else {
	info.port = port;
	}
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
		return false;
	}

	int retval = libwebsocket_service(websocketContext, 0);
	Com_Printf("libwebsocket_service returned %d\n", LOG_NET, retval);

	websocketInitialized = true;
	return true;
}


static void websocketShutdown() {
	assert(websocketInitialized);
	assert(websocketContext);

	libwebsocket_context_destroy(websocketContext);
	websocketContext = NULL;
	websocketInitialized = false;
}


static std::unique_ptr<Connection> createConnection(const netadr_t &to) {
	char addrBuf[3 * 4 + 5];
	snprintf(addrBuf, sizeof(addrBuf), "%u.%u.%u.%u", to.ip[0], to.ip[1], to.ip[2], to.ip[3]);
	struct libwebsocket *newWsi = libwebsocket_client_connect(websocketContext, addrBuf, ntohs(to.port), 0, "/", addrBuf, addrBuf, "quake2", -1);
	if (newWsi == NULL) {
		return std::unique_ptr<Connection>();
	}

	return std::unique_ptr<Connection>(new Connection(to, newWsi));
}


#else  // EMSCRIPTEN


struct Connection {
	netadr_t addr;


	explicit Connection(netadr_t addr_)
	: addr(addr_)
	{
	}


	~Connection()
	{
		STUBBED("~Connection");
	}
};


static bool createWebsocketContext(int port) {
	assert(!websocketInitialized);

	// TODO: pre-establish server connection
	STUBBED("createWebsocketContext");

	websocketInitialized = true;
	return true;
}


static void websocketShutdown() {
	assert(websocketInitialized);
	websocketInitialized = false;
}


static std::unique_ptr<Connection> createConnection(const netadr_t &to) {
	return std::unique_ptr<Connection>();
}
#endif  // EMSCRIPTEN


char *NET_ErrorString (void);
void Net_Restart_f (void);
void Net_Stats_f (void);
qboolean	NET_StringToSockaddr (const char *s, struct sockaddr *sadr);


#ifndef _WIN32
#define SOCKET unsigned int
#define INVALID_SOCKET -1
#endif


void closesocket(int socket) {
}


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


int NET_Client_Sleep (int msec)
{
	STUBBED("NET_Client_Sleep");
	return 0;
}


void NET_Common_Init (void)
{
	net_ignore_icmp = Cvar_Get ("net_ignore_icmp", "0", 0);

	Cmd_AddCommand ("net_restart", Net_Restart_f);
	Cmd_AddCommand ("net_stats", Net_Stats_f);
}


/*
====================
NET_Config

A single player game will only use the loopback code
====================
*/
int	NET_Config (int toOpen)
{
	static	int	old_config;

	int i = old_config;

	if (old_config == toOpen)
		return i;

	old_config |= toOpen;

	if (toOpen == NET_NONE)
	{
		if (websocketInitialized) {
			websocketShutdown();
		}

		server_port = 0;

		old_config = NET_NONE;
	}

	int flags = toOpen;

	net_total_in = net_packets_in = net_total_out = net_packets_out = 0;
	net_inittime = (unsigned int)time(NULL);

	cvar_t	*ip = Cvar_Get ("ip", "localhost", CVAR_NOSET);

	int dedicated = Cvar_IntValue ("dedicated");

	int		port;
	if (flags & NET_SERVER)
	{
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

			bool failed = false;
			// shut down old context
			if (websocketInitialized) {
				websocketShutdown();
			}

			if (!createWebsocketContext(port)) {
				failed = true;
				server_port = 0;
			}

			if (failed && dedicated)
				Com_Error (ERR_FATAL, "Couldn't allocate dedicated server IP port on %s:%d. Another application is probably using it.", ip->string, port);
		}
	}

	// dedicated servers don't need client ports
	if (dedicated)
		return i;

	bool failed = false;
	if (!websocketInitialized) {
		failed = !createWebsocketContext(PORT_ANY);
	}

	if (failed)
		Com_Error (ERR_DROP, "Couldn't allocate client IP port.");

	return i;
}


qboolean	NET_GetLoopPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	loopback_t	*loop = &loopbacks[sock];

	if (loop->send - loop->get > MAX_LOOPBACK)
		loop->get = loop->send - MAX_LOOPBACK;

	if (loop->get >= loop->send)
		return false;

	int i = loop->get & (MAX_LOOPBACK-1);
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


int	NET_GetPacket (netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
	if (NET_GetLoopPacket (sock, net_from, net_message))
		return 1;

	if (!websocketInitialized) {
		// not initialized yet
		return 0;
	}

#ifndef EMSCRIPTEN

	// TODO: what does the return value mean? do we care?
	libwebsocket_service(websocketContext, 0);

#endif  // EMSCRIPTEN

	STUBBED("NET_GetPacket");

	return 0;
}


uint32 NET_htonl (uint32 ip)
{
	return htonl (ip);
}


void NET_Init (void)
{
	NET_Common_Init ();
	net_no_recverr = Cvar_Get ("net_no_recverr", "0", 0);
}


char	*NET_inet_ntoa (uint32 ip)
{
	return inet_ntoa (*(struct in_addr *)&ip);
}


uint32 NET_ntohl (uint32 ip)
{
	return ntohl (ip);
}


void Net_Restart_f (void)
{
	int old = NET_Config (NET_NONE);
	NET_Config (old);
}


void NET_SendLoopPacket (netsrc_t sock, int length, const void *data)
{
	loopback_t	*loop = &loopbacks[sock^1];

	int i = loop->send & (MAX_LOOPBACK-1);
	loop->send++;

	memcpy (loop->msgs[i].data, data, length);
	loop->msgs[i].datalen = length;
}


int NET_SendPacket (netsrc_t sock, int length, const void *data, netadr_t *to)
{
	assert(to != NULL);

	if (to->type == NA_IP)
	{
		// if network not initialized return 0
		if (!websocketInitialized) {
			return 0;
		}
	}
	else if ( to->type == NA_LOOPBACK )
	{
		NET_SendLoopPacket (sock, length, data);
		return 1;
	}
	else if (to->type == NA_BROADCAST)
	{
		// websockets doesn't do broadcast
		return 0;
	}
	else
	{
		Com_Error (ERR_FATAL, "NET_SendPacket: bad address type");
		return 0;
	}

	assert(to->type == NA_IP);

	auto it = connections.find(*to);
	if (it == connections.end()) {
		// no connection, create it
		auto conn = createConnection(*to);
		if (!conn) {
			// TODO: log it?
			return 0;
		}
		bool success = false;
		std::tie(it, success) = connections.emplace(*to, std::move(conn));
		assert(success);  // it wasn't there before so this can't fail
	}

	STUBBED("NET_SendPacket");

	net_packets_out++;
	net_total_out += length;

	return 1;
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


// sleeps msec or until net socket is ready
#ifndef NO_SERVER
void NET_Sleep(int msec)
{
	extern cvar_t *dedicated;
	//extern qboolean stdin_active;

	if (!server_port || !dedicated->intvalue)
		return; // we're not a server, just run full speed

	//Com_Printf ("NET_Sleep (%d)\n", LOG_GENERAL, msec);

	STUBBED("NET_Sleep");
}
#endif


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
	memset (sadr, 0, sizeof(*sadr));

	//r1: better than just the first digit for ip validity :)
	const char *p = s;
	int	isip = 0;
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

	char	copy[128];
	
	//r1: CHECK THE GODDAMN BUFFER SIZE... sigh yet another overflow.
	Q_strncpy (copy, s, sizeof(copy)-1);

	// strip off a trailing :port if present
	for (char	*colon = copy ; colon[0] ; colon++)
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
		struct hostent	*h;
		if (! (h = gethostbyname(copy)) )
			return false;
		*(int *)&((struct sockaddr_in *)sadr)->sin_addr = *(int *)h->h_addr_list[0];
	}
	
	return true;
}


#endif  // defined(USE_LIBWEBSOCKETS) || defined(EMSCRIPTEN)
