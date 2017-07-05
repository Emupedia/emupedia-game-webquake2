//
//  humblenet_socket.h
//
//  Created by Chris Rudd on 3/9/16.
//

#ifndef HUMBLENET_SOCKET_H
#define HUMBLENET_SOCKET_H

#include "humblenet_p2p.h"

#if defined EMSCRIPTEN && !defined NO_NATIVE_SOCKETS
#define NO_NATIVE_SOCKETS
#endif

#ifdef NO_NATIVE_SOCKETS
/*
struct hostent {
	char FAR      *h_name;
	char FAR  FAR **h_aliases;
	short         h_addrtype;
	short         h_length;
	char FAR  FAR **h_addr_list;
};
 */

#include <netdb.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>

#else

#include <unistd.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <errno.h>

#endif

#ifndef EMSCRIPTEN
/*
 * NATIVE-ONLY
 *
 * This performs a standard select, but allows humblnet to register additional fds so its internal processing can still occur
 */
HUMBLENET_API int HUMBLENET_CALL humblenet_select(int nfds, fd_set *readfds, fd_set *writefds,
								   fd_set *exceptfds, struct timeval *timeout);
#endif


static int hs_recvfrom( int sock, void* buffer, int length, int flags, struct sockaddr* addr, uint32* addr_len ) {
	PeerId peer;
	int ret =  humblenet_p2p_recvfrom(buffer, length, &peer, 0 );
	((struct sockaddr_in*)addr)->sin_addr.s_addr = htonl(peer);
	// we dont support ports...yet....
	((struct sockaddr_in*)addr)->sin_port = 0;
	if( ret > 0 )
		return ret;
	else if( ret == -1 ) {
		errno = ECONNRESET;
		return ret;
	} else {
		errno = EWOULDBLOCK;
		ret = -1;
	}

#ifndef NO_NATIVE_SOCKETS
	if( sock != -1 )
	{
		ret = recvfrom(sock, buffer, length, flags, addr, addr_len );
	}
#endif
	return ret;
}

static int hs_sendto( int sock, const void* buffer, int length, int flags, struct sockaddr* addr, uint32 addr_len ) {
#ifndef NO_NATIVE_SOCKETS
	uint32_t ip = ntohl(((struct sockaddr_in*)addr)->sin_addr.s_addr) & 0xFF000000;
	if( sock != -1 && ip != 0 && ip != 0x80000000 ) {
		return sendto( sock, buffer, length, flags, addr, addr_len );
	}
#endif
	int ret = humblenet_p2p_sendto( buffer, length, ntohl(((struct sockaddr_in*)addr)->sin_addr.s_addr), 0 );
	if( ret == -1 )
		errno = ECONNRESET;
	return ret;
}

static int hs_select( int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
#ifdef EMSCRIPTEN
	return 0;
#else
	return humblenet_select( nfds, readfds, writefds, exceptfds, timeout );
#endif
}

struct hostent* hs_gethostbyname( const char* s ) {
	static const char* humblenet_domain = ".humblenet";
	int   humblenet_domain_length = strlen( humblenet_domain );
	
	static struct hostent buff[5];
	static struct in_addr addr;
	static char* list[5] = {0,0,0,0,0};
	
	struct hostent* ret = NULL;
	
	if( (strlen(s) < humblenet_domain_length) || (0 != Q_strncasecmp(s+strlen(s)-humblenet_domain_length, humblenet_domain, humblenet_domain_length)) ) {
#ifdef NO_NATIVE_SOCKETS
	#ifdef _WIN32
		WSASetLastError( WSAHOST_NOT_FOUND );
	#else
		h_errno = HOST_NOT_FOUND;
	#endif
		return NULL;
#else
		return gethostbyname(s);
#endif
	} else {
		// strip the domain suffix
		static char stemp[256] = {0};
		Q_strncpy(stemp, s, sizeof(stemp)-1);
		stemp[ strlen(s) - humblenet_domain_length ] = '\0';
		s = stemp;
	}
	
	if( s[0] == 'p' && s[1] == 'e' && s[2] == 'e' && s[3] == 'r' && s[4] == '_' ) {
		ret = buff;
		ret->h_addrtype = AF_INET;
		ret->h_length = 4;
		ret->h_aliases = list+1;
		
		ret->h_addr_list = list;
		
		ret->h_addr_list[0] = (char*)&addr;
		addr.s_addr = htonl( atoi( s+5 ) );
	}
	else {
		ret = buff;
		ret->h_addrtype = AF_INET;
		ret->h_length = 4;
		ret->h_aliases = list+1;
		
		ret->h_addr_list = list;
		
		ret->h_addr_list[0] = (char*)&addr;
		addr.s_addr = htonl( humblenet_p2p_virtual_peer_for_alias( s ) );
	}
	
	
	return ret;
}

#define select			hs_select
#define recvfrom		hs_recvfrom
#define sendto			hs_sendto
#define gethostbyname	hs_gethostbyname

#ifdef NO_NATIVE_SOCKETS

#define HUMBLENET_SOCKET		-2

#define close( x )				( 0 ) //( x == -1 ? 0 : close(x) )
#define socket(...)				( HUMBLENET_SOCKET )
#define ioctl( x, ... )			( x == HUMBLENET_SOCKET ? 0 : ioctl( x, __VA_ARGS__ ) )
#define setsockopt( x, ... )	( x == HUMBLENET_SOCKET ? 0 : setsockopt( x, __VA_ARGS__ ) )
#define bind( x, ... )			( x == HUMBLENET_SOCKET ? 0 : bind( x, __VA_ARGS__ ) )

#endif
// END


#endif /* HUMBLENET_SOCKET_H */
