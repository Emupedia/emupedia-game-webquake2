extern "C" {


void __afl_manual_init(void);


#include "../qcommon/qcommon.h"


struct hostent {
	char  *h_name;            /* official name of host */
	char **h_aliases;         /* alias list */
	int    h_addrtype;        /* host address type */
	int    h_length;          /* length of address */
	char **h_addr_list;       /* list of addresses */
};


}  // extern "C"


#include <sys/stat.h>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <unordered_map>
#include <string>


#define STUBBED(x) do { \
	static bool seen_this = false; \
	if (!seen_this) { \
		seen_this = true; \
		printf("STUBBED: %s at %s (%s:%d)\n", \
		x, __FUNCTION__, __FILE__, __LINE__); \
	} \
	} while (0)


static std::unordered_map<std::string, struct hostent> hosts;


extern "C" {


static off_t fuzzSize = 0;
static off_t fuzzOffs = 0;
static char *fuzzBuf = NULL;


static void fuzzExpectLeft(off_t numBytes) {
	off_t bytesLeft = (fuzzSize - fuzzOffs);

	if (bytesLeft < numBytes) {
		// end of input
		Com_Quit();
	}
}


static void fuzzRead(char *dest, size_t size) {
	fuzzExpectLeft(size);
	memcpy(dest, fuzzBuf + fuzzOffs, size);
	fuzzOffs += size;
}


static uint16_t fuzzReadUint16() {
	uint16_t value = 0;
	fuzzRead(reinterpret_cast<char *>(&value), sizeof(value));

	return value;
}


static unsigned int net_inittime = 0;

static unsigned long long net_total_in = 0;
static unsigned long long net_total_out = 0;
static unsigned long long net_packets_in = 0;
static unsigned long long net_packets_out = 0;

int server_port = 0;

static int ip_sockets[2] = { 0, 0 };


struct in_addr {
	unsigned long s_addr;  // load with inet_aton()
};


struct sockaddr_in {
	short            sin_family;   // e.g. AF_INET
	unsigned short   sin_port;     // e.g. htons(3490)
	struct in_addr   sin_addr;     // see struct in_addr, below
	char             sin_zero[8];  // zero this if you want to
};


struct sockaddr {
	unsigned short    sa_family;    // address family, AF_xxx
	char              sa_data[14];  // 14 bytes of protocol address
};


typedef uint32_t in_addr_t;


static uint32_t htonl(uint32_t x) { return x; }

static uint32_t ntohl(uint32_t x) { return x; }

static uint16_t htons(uint16_t x) { return x; }

static uint16_t ntohs(uint16_t x) { return x; }


static struct hostent *gethostbyname(const char *hostname_) {
	std::string hostname(hostname_);

	auto it = hosts.find(hostname);
	if (it != hosts.end()) {
		return &(*it).second;
	}

	struct hostent newHostEnt;

	newHostEnt.h_name = new char[hostname.size()];
	strncpy(newHostEnt.h_name, hostname.c_str(), hostname.size());

	fuzzRead(reinterpret_cast<char *>(&newHostEnt.h_addrtype), sizeof(newHostEnt.h_addrtype));

	newHostEnt.h_addr_list = new char*[2];
	newHostEnt.h_addr_list[0] = newHostEnt.h_name;
	newHostEnt.h_addr_list[1] = NULL;

	// TODO: fuzz these too
	newHostEnt.h_aliases = NULL;
	newHostEnt.h_length = 0;

	auto retval = hosts.emplace(std::move(hostname), newHostEnt);
	assert(retval.second);
	it = retval.first;

	return &(*it).second;
}


static in_addr_t inet_addr(const char *cp) {
	STUBBED("inet_addr");

	return 0;
}


static char *inet_ntoa(struct in_addr in) {
	STUBBED("inet_ntoa");

	return NULL;
}


#define AF_INET 1


void Net_Stats_f (void)
{
}


//Aiee...
#include "../qcommon/net_common.c"


int NET_GetPacket(netsrc_t sock, netadr_t *net_from, sizebuf_t *net_message)
{
#ifndef DEDICATED_ONLY
	if (NET_GetLoopPacket (sock, net_from, net_message))
		return 1;
#endif

	int net_socket = ip_sockets[sock];

	if (!net_socket)
		return 0;

	// TODO: failed packets?

	uint16_t packetSize = fuzzReadUint16();

	packetSize = std::min(packetSize, static_cast<uint16_t>(net_message->maxsize));

	fuzzRead(reinterpret_cast<char *>(net_from), sizeof(*net_from));
	fuzzRead(reinterpret_cast<char *>(net_message->data), packetSize);
	net_message->cursize = packetSize;

	return 1;
}


void NET_Init (void)
{
	Cvar_SetValue("nostdin", 1.0f);

	NET_Common_Init ();

#ifdef __AFL_HAVE_MANUAL_INIT
	__afl_manual_init();
#endif  // __AFL_HAVE_MANUAL_INIT

	cvar_t *afl_fuzz_file = Cvar_Get("afl_fuzz_file", NULL, 0);
	if (!afl_fuzz_file) {
		Com_Printf("afl_fuzz_file not set\n", LOG_NET);
		// so we stop uselessly running the fuzzer
		__builtin_trap();
	}

	FILE *f = fopen(afl_fuzz_file->string, "rb");
	if (!f) {
		Com_Printf("Failed to open afl_fuzz_file \"%s\" %d \"%s\" \n", LOG_NET, afl_fuzz_file->string, errno, strerror(errno));
		// so we stop uselessly running the fuzzer
		__builtin_trap();
	}

	struct stat statbuf;
	memset(&statbuf, 0, sizeof(struct stat));
	int retval = fstat(fileno(f), &statbuf);
	if (retval != 0) {
		Com_Printf("Failed to stat afl_fuzz_file \"%s\" %d \"%s\" \n", LOG_NET, afl_fuzz_file->string, errno, strerror(errno));
		fclose(f);
		// so we stop uselessly running the fuzzer
		__builtin_trap();
	}

	fuzzSize = statbuf.st_size;
	fuzzBuf = new char[fuzzSize];
	fread(fuzzBuf, 1, fuzzSize, f);

	fclose(f);
}


int NET_IPSocket (char *net_interface, int port)
{
	static int nextSocket = 1;

	return nextSocket++;
}


int NET_SendPacket (netsrc_t sock, int length, const void *data, netadr_t *to)
{
	STUBBED("NET_SendPacket");

	return 0;
}


void NET_Shutdown (void)
{
	NET_Config (NET_NONE);	// close sockets
}


}  // extern "C"

