#include "humblenet.h"
#include "humblenet_p2p.h"
#include "humblenet_p2p_internal.h"
#include "humblenet_alias.h"

#define NOMINMAX

#include <cassert>

#include <cstdlib>
#include <string>
#include <cstring>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

#if defined(__APPLE__) || defined(__linux__)
	#include <sys/utsname.h>
#endif

#if defined(WIN32)
#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#else
	#include <sys/time.h>
#endif

#include "humblepeer.h"

#include "libsocket.h"

#include "humblenet_p2p_internal.h"
#include "humblenet_utils.h"

#define USE_STUN

HumbleNetState humbleNetState;

#ifdef WIN32
#if defined(_MSC_VER) || defined(_MSC_EXTENSIONS)
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000Ui64
#else
#define DELTA_EPOCH_IN_MICROSECS  11644473600000000ULL
#endif
uint64_t sys_milliseconds(void)
{
	FILETIME ft;
	ULARGE_INTEGER temp;

	GetSystemTimeAsFileTime(&ft);
	memcpy(&temp, &ft, sizeof(temp));

	return (temp.QuadPart - DELTA_EPOCH_IN_MICROSECS) / 100000;
}

#else
uint64_t sys_milliseconds (void)
{
	struct timeval tp;
	struct timezone tzp;
	static uint64_t	secbase = 0;
	
	gettimeofday(&tp, &tzp);
	
	if (!secbase)
	{
		secbase = tp.tv_sec;
		return tp.tv_usec/1000;
	}
	
	return (tp.tv_sec - secbase)*1000 + tp.tv_usec/1000;
}
#endif

void blacklist_peer( PeerId peer ) {
	humbleNetState.peerBlacklist.insert( std::make_pair( peer, sys_milliseconds() + 5000 ) );
}

/*
 * Deterimine if the peer is currently blacklisted.
 *
 * Side effect: will clear expired blacklist entries.
 */
bool is_peer_blacklisted( PeerId peer_id ) {
	auto it = humbleNetState.peerBlacklist.find(peer_id);
	if( it == humbleNetState.peerBlacklist.end() )
		return false;
	
	if( it->second <= sys_milliseconds() ) {
		humbleNetState.peerBlacklist.erase(it);
		return true;
	}
	
	return true;
}

/*
 * See if we can try a connection to this peer
 *
 * Side effect: sets error to reason on failure
 */
bool can_try_peer( PeerId peer_id ) {
	if (!humbleNetState.p2pConn) {
		humblenet_set_error("Signaling connection not established");
		// no signaling connection
		return false;
	}
	if (humbleNetState.myPeerId == 0) {
		humblenet_set_error("No peer ID from server");
		return false;
	}
	if (humbleNetState.pendingPeerConnectionsOut.find(peer_id)
		!= humbleNetState.pendingPeerConnectionsOut.end()) {
		// error, already a pending outgoing connection to this peer
		humblenet_set_error("already a pending connection to peer");
		LOG("humblenet_connect_peer: already a pending connection to peer %u\n", peer_id);
		return false;
	}
	if( is_peer_blacklisted(peer_id) ) {
		// peer is currently black listed
		humblenet_set_error("peer blacklisted");
		LOG("humblenet_connect_peer: peer blacklisted %u\n", peer_id);
		return false;
	}
	
	return true;
}


// BEGIN CONNECTION HANDLING

void humblenet_connection_set_closed( Connection* conn ) {
    if( conn->socket ) {
        // clear our state first so callbacks dont work on us.
        humbleNetState.connections.erase(conn->socket);
        internal_set_data(conn->socket, NULL );
        internal_close_socket(conn->socket);
        conn->socket = NULL;
    }
	
	if( conn->inOrOut == Incoming ) {
		blacklist_peer( conn->otherPeer );
	}
	
    conn->status = HUMBLENET_CONNECTION_CLOSED;
    
    LOG("Marking connections closed: %u\n", conn->otherPeer );
    
    // make sure were not in any lists...
    erase_value( humbleNetState.connections, conn );
    humbleNetState.pendingNewConnections.erase( conn );
    humbleNetState.pendingDataConnections.erase( conn );
//    humbleNetState.remoteClosedConnections.erase( conn );
    erase_value( humbleNetState.pendingPeerConnectionsOut, conn );
    erase_value( humbleNetState.pendingPeerConnectionsIn, conn );
    erase_value( humbleNetState.pendingAliasConnectionsOut, conn );

    // mark it as being close pending.
    humbleNetState.remoteClosedConnections.insert( conn );
}


ha_bool humblenet_connection_is_readable(Connection *connection) {
    return !connection->recvBuffer.empty();
}


ha_bool humblenet_connection_is_writable(Connection *connection) {
    assert(connection != NULL);
    if (connection->status == HUMBLENET_CONNECTION_CONNECTED) {
        return connection->writable;
    }
    return false;
}


int humblenet_connection_write(Connection *connection, const void *buf, uint32_t bufsize) {
    assert(connection != NULL);
    
    switch (connection->status) {
        case HUMBLENET_CONNECTION_CLOSED:
            // connection has been closed
            assert(connection->socket == NULL);
            return -1;
            
        case HUMBLENET_CONNECTION_CONNECTING:
            assert(connection->socket != NULL);
            return 0;
            
        case HUMBLENET_CONNECTION_CONNECTED:
            assert(connection->socket != NULL);
            return internal_write_socket( connection->socket, buf, bufsize );
    }
	return -1;
}


int humblenet_connection_read(Connection *connection, void *buf, uint32_t bufsize) {
    assert(connection != NULL);
    
    switch (connection->status) {
        case HUMBLENET_CONNECTION_CLOSED:
            // connection has been closed
            assert(connection->socket == NULL);
            return -1;
            
        case HUMBLENET_CONNECTION_CONNECTING:
            assert(connection->socket != NULL);
            return 0;
            
        case HUMBLENET_CONNECTION_CONNECTED:
            assert(connection->socket != NULL);
            
            if( connection->recvBuffer.empty() )
                return 0;
            
            bufsize = std::min<uint32_t>(bufsize, connection->recvBuffer.size() );
            memcpy(buf, &connection->recvBuffer[0], bufsize);
            connection->recvBuffer.erase(connection->recvBuffer.begin()
                                         , connection->recvBuffer.begin() + bufsize);
            
            if( ! connection->recvBuffer.empty() )
                humbleNetState.pendingDataConnections.insert(connection);
            else
                humbleNetState.pendingDataConnections.erase(connection);
            
            return bufsize;
    }
	return -1;
}

ConnectionStatus humblenet_connection_status(Connection *connection) {
    assert(connection != NULL);
    
    return connection->status;
}

Connection *humblenet_connect_websocket(const char *server_addr) {
    humblenet_set_error("Websocket support deprecated");
    return NULL;
}


Connection *humblenet_connect_peer(PeerId peer_id) {
    if( ! can_try_peer( peer_id ) )
		return NULL;
    
    Connection *connection = new Connection(Outgoing);
    connection->otherPeer = peer_id;
    connection->socket = internal_create_webrtc(humbleNetState.context);
    internal_set_data(connection->socket, connection);
    
    humbleNetState.pendingPeerConnectionsOut.emplace(peer_id, connection);
    
    if( ! internal_create_offer( connection->socket ) ) {
        humblenet_set_error("Unable to generate sdp offer");
        humblenet_connection_close(connection);
        return NULL;
    }
    
    LOG("connecting to peer: %d\n", peer_id);
    return connection;
}


void humblenet_connection_close(Connection *connection) {
    assert(connection != NULL);

    humblenet_connection_set_closed(connection);

    internal_alias_remove_connection( connection );
    
    humbleNetState.remoteClosedConnections.erase(connection);
    
    delete connection;
}

PeerId humblenet_connection_get_peer_id(Connection *connection) {
    assert(connection != NULL);
    
    return connection->otherPeer;
}


// called to return the sdp offer
int on_sdp( internal_socket_t* s, const char* offer, void* user_data ) {
    Connection* conn = reinterpret_cast<Connection*>(user_data);
    
    if( conn == NULL ) {
        LOG("Got socket w/o state?\n");
        return -1;
    }
    
    assert( conn->status == HUMBLENET_CONNECTION_CONNECTING );
    
    // no trickle ICE
    int flags = 0x2;
    
    if( conn->inOrOut == Incoming ) {
        LOG("P2PConnect SDP sent %u response offer = \"%s\"\n", conn->otherPeer, offer);
        sendP2PResponse(humbleNetState.p2pConn.get(), conn->otherPeer, offer);
    } else {
        LOG("outgoing SDP sent %u offer: \"%s\"\n", conn->otherPeer, offer);
        sendP2PConnect(humbleNetState.p2pConn.get(), conn->otherPeer, flags, offer);
    }
    return 0;
}

// called to send a candidate
int on_ice_candidate( internal_socket_t* s, const char* offer, void* user_data ) {
    Connection* conn = reinterpret_cast<Connection*>(user_data);
    
    if( conn == NULL ) {
        LOG("Got socket w/o state?\n");
        return -1;
    }
    
    assert( conn->status == HUMBLENET_CONNECTION_CONNECTING );
    
    LOG("Sending ice candidate to peer: %u, %s\n", conn->otherPeer, offer );
    sendICECandidate(humbleNetState.p2pConn.get(), conn->otherPeer, offer);
    
    return 0;
}

// called for incomming connections to indicate the connection process is completed.
int on_accept (internal_socket_t* s, void* user_data) {
    auto it = humbleNetState.connections.find( s );
    
    // TODO: Rework this ?
    Connection* conn = NULL;
    if( it == humbleNetState.connections.end() ) {
        // its a websocket connection
        conn = new Connection( Incoming, s );
        
        // track the connection, ALL connections will reside here.
        humbleNetState.connections.insert( std::make_pair( s, conn ) );
    } else {
        // should be a webrtrc connection (as we create the socket inorder to start the process)
        conn = it->second;
    }
    
    // TODO: This should be "waiting for accept"
    conn->status = HUMBLENET_CONNECTION_CONNECTED;
    
    // expose it as an incomming connection to be accepted...
    humbleNetState.pendingNewConnections.insert( conn );
    
    LOG("accepted peer: %d\n", conn->otherPeer );
    
    return 0;
}

// called for outgoing connections to indicate the connection process is completed.
int on_connect (internal_socket_t* s, void* user_data) {
    assert( user_data );
    
    auto it = humbleNetState.connections.find( s );
    
    // TODO: Rework this ?
    Connection* conn = reinterpret_cast<Connection*>(user_data);
    if( it == humbleNetState.connections.end() ) {
        // track the connection, ALL connections will reside here.
        humbleNetState.connections.insert( std::make_pair( s, conn ) );
    }
    
    assert( conn->status == HUMBLENET_CONNECTION_CONNECTING );
    
    // outgoing always initiates the channel
    if( conn->inOrOut == Outgoing ) {
        if( ! internal_create_channel(s, "dataChannel") )
            return -1;
    }
    
    LOG("connected peer: %d\n", conn->otherPeer );
    
    return 0;
}

int on_accept_channel( internal_socket_t* s, const char* name, void* user_data ) {
    assert( user_data );
    
    Connection* conn = reinterpret_cast<Connection*>(user_data);
    
    assert( conn->status == HUMBLENET_CONNECTION_CONNECTING );
    conn->status = HUMBLENET_CONNECTION_CONNECTED;
    
    LOG("accepted channel: %d:%s\n", conn->otherPeer, name );
    return 0;
}

int on_connect_channel( internal_socket_t* s, const char* name, void* user_data ) {
    assert( user_data );
    
    Connection* conn = reinterpret_cast<Connection*>(user_data);
    
    assert( conn->status == HUMBLENET_CONNECTION_CONNECTING );
    conn->status = HUMBLENET_CONNECTION_CONNECTED;
    
    LOG("connected channel: %d:%s\n", conn->otherPeer, name );
    return 0;
}

// called each time data is received.
int on_data( internal_socket_t* s, const void* data, int len, void* user_data ) {
    assert( user_data );
    
    Connection* conn = reinterpret_cast<Connection*>(user_data);
    
    assert( conn->status == HUMBLENET_CONNECTION_CONNECTED );
    
    if( conn->recvBuffer.empty() ) {
        assert( humbleNetState.pendingDataConnections.find(conn) == humbleNetState.pendingDataConnections.end() );
        humbleNetState.pendingDataConnections.insert(conn);
    }
    
    conn->recvBuffer.insert( conn->recvBuffer.end(), reinterpret_cast<const char*>(data),
                            reinterpret_cast<const char*>(data)+len);
    return 0;
}

// called to indicate the connection is wriable.
int on_writable (internal_socket_t* s, void* user_data) {
    auto it = humbleNetState.connections.find( s );
    
    if( it != humbleNetState.connections.end() ) {
        // its not here anymore
        return -1;
    }
    
    Connection* conn = it->second;
    conn->writable = true;
    
    return 0;
}

// called when the conenction is terminated (from either side)
int on_disconnect( internal_socket_t* s, void* user_data ) {
    
    // find Connection object
    auto it = humbleNetState.connections.find(s);
    
    if (it != humbleNetState.connections.end()) {
        // it's still in connections map, not user-initiated close
        // find Connection object and set wsi to NULL
        // to signal that it's dead
        // user code will then call humblenet_close to dispose of
        // the Connection object
        
        humblenet_connection_set_closed( it->second );
    } else if( user_data != NULL ) {
        // just to be sure...
        humblenet_connection_set_closed( reinterpret_cast<Connection*>( user_data ) );
    }
    // if it wasn't in the table, no need to do anything
    // humblenet_close has already disposed of everything else
    return 0;
}

int on_destroy( internal_socket_t* s, void* user_data ) {
    // if no user_data attached, then nothing to cleanup.
    if( ! user_data )
        return 0;
    
    // make sure we are fully detached...
    Connection* conn = reinterpret_cast<Connection*>(user_data);
    humblenet_connection_set_closed( conn );
    
    return 0;
}

// END CONNECTION HANDLING



// BEGIN SIGNALING

static ha_bool p2pSignalProcess(const humblenet::HumblePeer::Message *msg, void *user_data);

namespace humblenet {
	ha_bool sendP2PMessage(P2PSignalConnection *conn, const uint8_t *buff, size_t length)
	{
		assert(conn != NULL);
		if (conn->wsi == NULL) {
			// this really shouldn't happen but apparently it can
			// if the game hasn't properly opened a signaling connection
			return false;
		}

		int ret = internal_write_socket(conn->wsi, (const void*)buff, length );
		assert( ret == length );

		return true;
	}

    // called for incomming connections to indicate the connection process is completed.
    int on_accept (internal_socket_t* s, void* user_data) {
        // we dont accept incomming connections
        return -1;
    }
    
    // called for outgoing commentions to indicate the connection process is completed.
    int on_connect (internal_socket_t* s, void* user_data) {
        assert(s != NULL);
        if (!humbleNetState.p2pConn) {
            // there is no signaling connection in humbleNetState
            // this one must be obsolete, close it
            return -1;
        }
        
        // temporary copy, doesn't transfer ownership
        P2PSignalConnection *conn = humbleNetState.p2pConn.get();
        if (conn->wsi != s) {
            // this connection is not the one currently active in humbleNetState.
            // this one must be obsolete, close it
            return -1;
        }
        // platform info
        std::string pinfo;
#if defined(__APPLE__) || defined(__linux__)
        struct utsname buf;
        memset(&buf, 0, sizeof(struct utsname));
        int retval = uname(&buf);
        if (retval == 0)
        {
            pinfo.append("Sysname: ");
            pinfo.append(buf.sysname);
            pinfo.append(", Release: ");
            pinfo.append(buf.release);
            pinfo.append(", Version: ");
            pinfo.append(buf.version);
            pinfo.append(", Machine: ");
            pinfo.append(buf.machine);
        }
        else
        {
            LOG("Failed getting system info with uname\n");
        }
#elif defined(_WIN32)
        OSVERSIONINFO osvi;
        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        GetVersionEx(&osvi);
        pinfo.append("Windows " + std::to_string(osvi.dwMajorVersion) + "." + std::to_string(osvi.dwMinorVersion) + ", build " + std::to_string(osvi.dwBuildNumber));
        
        SYSTEM_INFO si;
        ZeroMemory(&si, sizeof(SYSTEM_INFO));
        GetSystemInfo(&si);
        switch (si.wProcessorArchitecture)
        {
            case PROCESSOR_ARCHITECTURE_INTEL:
                pinfo.append(", Intel x86");
                break;
            case PROCESSOR_ARCHITECTURE_IA64:
                pinfo.append(", Intel Itanium");
                break;
            case PROCESSOR_ARCHITECTURE_AMD64:
                pinfo.append(", AMD or Intel x64");
                break;
            default:
                pinfo.append(", non-desktop architecture");
                break;
        }
#endif
        
        uint8_t flags = 0;
        if (humbleNetState.webRTCSupported) {
            // No trickle ICE on native
            flags = (0x1 | 0x2);
        }
		std::map<std::string, std::string> attributes;
		ha_bool helloSuccess = sendHelloServer(conn, flags, humbleNetState.gameToken, humbleNetState.gameSecret, humbleNetState.authToken, humbleNetState.reconnectToken, attributes);
        if (!helloSuccess) {
            // something went wrong, close the connection
            // don't humblenet_set_error, sendHelloServer should have done that
            humbleNetState.p2pConn.reset();
            return -1;
        }
        return 0;
    }
    
    // called each time data is received.
    int on_data( internal_socket_t* s, const void* data, int len, void* user_data ) {
        assert(s != NULL);
        
        if (!humbleNetState.p2pConn) {
            // there is no signaling connection in humbleNetState
            // this one must be obsolete, close it
            return -1;
        }
        
        // temporary copy, doesn't transfer ownership
        P2PSignalConnection *conn = humbleNetState.p2pConn.get();
        if (conn->wsi != s) {
            // this connection is not the one currently active in humbleNetState.
            // this one must be obsolete, close it
            return -1;
        }
        
        //        LOG("Data: %d -> %s\n", len, std::string((const char*)data,len).c_str());
        
        conn->recvBuf.insert(conn->recvBuf.end()
                             , reinterpret_cast<const char *>(data)
                             , reinterpret_cast<const char *>(data) + len);
        ha_bool retval = parseMessage(conn->recvBuf, p2pSignalProcess, NULL);
        if (!retval) {
            // error while parsing a message, close the connection
            humbleNetState.p2pConn.reset();
            return -1;
        }
        return 0;
    }
    
    // called to indicate the connection is wriable.
    int on_writable (internal_socket_t* s, void* user_data) {
        assert(s != NULL);
        if (!humbleNetState.p2pConn) {
            // there is no signaling connection in humbleNetState
            // this one must be obsolete, close it
            return -1;
        }
        
        // temporary copy, doesn't transfer ownership
        P2PSignalConnection *conn = humbleNetState.p2pConn.get();
        if (conn->wsi != s) {
            // this connection is not the one currently active in humbleNetState.
            // this one must be obsolete, close it
            return -1;
        }
        
        if (conn->sendBuf.empty()) {
            // no data in sendBuf
            return 0;
        }
        
        int retval = internal_write_socket( conn->wsi, &conn->sendBuf[0], conn->sendBuf.size() );
        if (retval < 0) {
            // error while sending, close the connection
            // TODO: should try to reopen after some time
            humbleNetState.p2pConn.reset();
            return -1;
        }
        
        // successful write
        conn->sendBuf.erase(conn->sendBuf.begin(), conn->sendBuf.begin() + retval);
        
        return 0;
    }
    
    // called when the conenction is terminated (from either side)
    int on_disconnect( internal_socket_t* s, void* user_data ) {
        
        if( humbleNetState.p2pConn ) {
            if( s == humbleNetState.p2pConn->wsi ) {
                
                // handle retry...
                humbleNetState.p2pConn.reset();
                
                return 0;
            }
        }
        
        return 0;
    }
    
    
    void register_protocol(internal_context_t* context) {
        internal_callbacks_t callbacks;
        
        memset(&callbacks, 0, sizeof(callbacks));
        
        callbacks.on_accept = on_accept;
        callbacks.on_connect = on_connect;
        callbacks.on_data = on_data;
        callbacks.on_disconnect = on_disconnect;
        callbacks.on_writable = on_writable;
        
        internal_register_protocol( humbleNetState.context, "humblepeer", &callbacks );
    }
}

static ha_bool p2pSignalProcess(const humblenet::HumblePeer::Message *msg, void *user_data)
{
	using namespace humblenet;

	auto msgType = msg->message_type();

	switch (msgType) {
		case HumblePeer::MessageType::P2POffer:
		{
			auto p2p = reinterpret_cast<const HumblePeer::P2POffer*>(msg->message());
			PeerId peer = static_cast<PeerId>(p2p->peerId());

			if (humbleNetState.pendingPeerConnectionsIn.find(peer) != humbleNetState.pendingPeerConnectionsIn.end()) {
				// error, already a pending incoming connection to this peer
				LOG("p2pSignalProcess: already a pending connection to peer %u\n", peer);
				return true;
			}

			bool emulated = ( p2p->flags() & 0x1 );
			
			if (emulated || !humbleNetState.webRTCSupported) {
				// webrtc connections not supported
				assert(humbleNetState.p2pConn);
				sendPeerRefused(humbleNetState.p2pConn.get(), peer);
			}

			Connection *connection = new Connection(Incoming);
			connection->status = HUMBLENET_CONNECTION_CONNECTING;
			connection->otherPeer = peer;
			connection->socket = internal_create_webrtc(humbleNetState.context);
			internal_set_data( connection->socket, connection );

			humbleNetState.pendingPeerConnectionsIn.insert(std::make_pair(peer, connection));

			auto offer = p2p->offer();

			std::vector<char> payloadv(offer->begin(), offer->end());

			LOG("P2PConnect SDP got %u's offer = \"%s\"\n", peer, offer->c_str());
			if( ! internal_set_offer( connection->socket, payloadv.data() ) )
			{
				humblenet_connection_close( connection );

				sendPeerRefused(humbleNetState.p2pConn.get(), peer);
				}
		}
			break;

		case HumblePeer::MessageType::P2PAnswer:
		{
			auto p2p = reinterpret_cast<const HumblePeer::P2PAnswer*>(msg->message());
			PeerId peer = static_cast<PeerId>(p2p->peerId());

			auto it = humbleNetState.pendingPeerConnectionsOut.find(peer);
			if (it == humbleNetState.pendingPeerConnectionsOut.end()) {
				LOG("P2PResponse for connection we didn't initiate: %u\n", peer);
				return true;
			}

			Connection *conn = it->second;
			assert(conn != NULL);
			assert(conn->otherPeer == peer);

			assert(conn->socket != NULL);
			// TODO: deal with _CLOSED
			assert(conn->status == HUMBLENET_CONNECTION_CONNECTING);

			auto offer = p2p->offer();

			std::vector<char> payloadv(offer->begin(), offer->end());

			LOG("P2PResponse SDP got %u's response offer = \"%s\"\n", peer, offer->c_str());
			if( !internal_set_answer( conn->socket, payloadv.data() ) ) {
				humblenet_connection_set_closed( conn );
			}
		}
			break;

		case HumblePeer::MessageType::HelloClient:
		{
			auto hello = reinterpret_cast<const HumblePeer::HelloClient*>(msg->message());
			PeerId peer = static_cast<PeerId>(hello->peerId());

			if (humbleNetState.myPeerId != 0) {
				LOG("Error: got HelloClient but we already have a peer id\n");
				return true;
			}
			LOG("My peer id is %u\n", peer);
			humbleNetState.myPeerId = peer;

			humbleNetState.iceServers.clear();

			if (hello->iceServers()) {
				auto iceList = hello->iceServers();
				for (const auto& it : *iceList) {
					if (it->type() == HumblePeer::ICEServerType::STUNServer) {
						humbleNetState.iceServers.emplace_back(it->server()->str());
					} else if (it->type() == HumblePeer::ICEServerType::TURNServer) {
						auto username = it->username();
						auto pass = it->password();
						if (pass && username) {
							humbleNetState.iceServers.emplace_back(it->server()->str(), username->str(), pass->str());
						}
					}
				}
			} else {
				LOG("No STUN/TURN credentials provided by the server");
			}

			std::vector<const char*> stunServers;
			for (auto& it : humbleNetState.iceServers) {
				if (it.type == HumblePeer::ICEServerType::STUNServer) {
					stunServers.emplace_back(it.server.c_str());
				}
			}
			internal_set_stun_servers(humbleNetState.context, stunServers.data(), stunServers.size());
			//            internal_set_turn_server( server.c_str(), username.c_str(), password.c_str() );
		}
			break;

		case HumblePeer::MessageType::ICECandidate:
		{
			auto iceCandidate = reinterpret_cast<const HumblePeer::ICECandidate*>(msg->message());
			PeerId peer = static_cast<PeerId>(iceCandidate->peerId());

			auto it = humbleNetState.pendingPeerConnectionsIn.find( peer );

			if( it == humbleNetState.pendingPeerConnectionsIn.end() )
			{
				it = humbleNetState.pendingPeerConnectionsOut.find( peer );
				if( it == humbleNetState.pendingPeerConnectionsOut.end() )
				{
					// no connection waiting on a peer.
					return true;
				}
			}

			if( it->second->socket && it->second->status == HUMBLENET_CONNECTION_CONNECTING ) {
				auto offer = iceCandidate->offer();
				LOG("Got ice candidate from peer: %d, %s\n", it->second->otherPeer, offer->c_str() );
				
				internal_add_ice_candidate( it->second->socket, offer->c_str() );
			}
		}
			break;

		case HumblePeer::MessageType::P2PReject:
		{
			auto reject = reinterpret_cast<const HumblePeer::P2PReject*>(msg->message());
			PeerId peer = static_cast<PeerId>(reject->peerId());

			auto it = humbleNetState.pendingPeerConnectionsOut.find(peer);
			if (it == humbleNetState.pendingPeerConnectionsOut.end()) {
				switch (reject->reason()) {
					case HumblePeer::P2PRejectReason::NotFound:
						LOG("Peer %u does not exist\n", peer);
						break;
					case HumblePeer::P2PRejectReason::PeerRefused:
						LOG("Peer %u rejected our connection\n", peer);
						break;
				}
				return true;
			}

			Connection *conn = it->second;
			assert(conn != NULL);

			blacklist_peer(peer);
			humblenet_connection_set_closed(conn);
		}
			break;

		case HumblePeer::MessageType::P2PConnected:
		{
			auto connect = reinterpret_cast<const HumblePeer::P2PConnected*>(msg->message());

			LOG("Established connection to peer %u", connect->peerId());
		}
			break;

		case HumblePeer::MessageType::P2PDisconnect:
		{
			auto disconnect = reinterpret_cast<const HumblePeer::P2PDisconnect*>(msg->message());

			LOG("Disconnecting peer %u", disconnect->peerId());
		}
			break;

		case HumblePeer::MessageType::AliasResolved:
		{
			auto reolved = reinterpret_cast<const HumblePeer::AliasResolved*>(msg->message());

			internal_alias_resolved_to( reolved->alias()->c_str(), reolved->peerId() );
		}
			break;

		default:
			LOG("p2pSignalProcess unhandled %s\n", HumblePeer::EnumNameMessageType(msgType));
			break;
	}

	return true;
}

ha_bool humblenet_p2p_connect_server() {
    using namespace humblenet;
    
    if (humbleNetState.p2pConn) {
        humbleNetState.p2pConn->disconnect();
        humbleNetState.p2pConn.reset();
        humbleNetState.myPeerId = 0;
    }
    
    LOG("connecting to signaling server \"%s\" with gameToken \"%s\" \n", humbleNetState.signalingServerAddr.c_str(), humbleNetState.gameToken.c_str());
    
    humbleNetState.p2pConn.reset(new P2PSignalConnection);
    humbleNetState.p2pConn->wsi = internal_connect_websocket(humbleNetState.signalingServerAddr.c_str(), "humblepeer");
    
    if (humbleNetState.p2pConn->wsi == NULL) {
        humbleNetState.p2pConn.reset();
        // TODO: can we get more specific reason ?
        humblenet_set_error("WebSocket connection to signaling server failed");
        return false;
    }
    
    LOG("new wsi: %p\n", humbleNetState.p2pConn->wsi);
    
    humblenet_poll(0);
    return true;
}
// END SIGNALING


// BEGIN INITIALIZATION

ha_bool HUMBLENET_CALL humblenet_init() {
	return true;
}

ha_bool internal_p2p_register_protocol() {
    internal_callbacks_t callbacks;

    memset(&callbacks, 0, sizeof(callbacks));

    callbacks.on_sdp = on_sdp;
    callbacks.on_ice_candidate = on_ice_candidate;
    callbacks.on_accept = on_accept;
    callbacks.on_connect = on_connect;
    callbacks.on_accept_channel = on_accept_channel;
    callbacks.on_connect_channel = on_connect_channel;
    callbacks.on_data = on_data;
    callbacks.on_disconnect = on_disconnect;
    callbacks.on_writable = on_writable;

    humbleNetState.context = internal_init(  &callbacks );
    if (humbleNetState.context == NULL) {
        return false;
    }

    humblenet::register_protocol(humbleNetState.context);

    humbleNetState.webRTCSupported = internal_supports_webRTC( humbleNetState.context );

    return true;
}


#ifndef EMSCRIPTEN
extern "C" void poll_deinit();
#endif

void HUMBLENET_CALL humblenet_shutdown() {
    // Destroy all connections
    for( auto it = humbleNetState.connections.begin(); it != humbleNetState.connections.end(); ) {
        Connection* conn = it->second;
        ++it;
        humblenet_connection_close( conn );
    }
    // drop the server
    if( humbleNetState.p2pConn ) {
        humbleNetState.p2pConn->disconnect();
        humbleNetState.p2pConn.reset();
    }
    
    internal_deinit(humbleNetState.context);
    humbleNetState.context = NULL;
    
#ifndef EMSCRIPTEN
    poll_deinit();
#endif
}

// END INITIALIZATION

// BEGIN MISC

ha_bool HUMBLENET_CALL humblenet_p2p_supported() {
    // This function should really be "humblenet_has_webrtc"
    // but could be removed entirely from the API
    return humbleNetState.webRTCSupported;
}

const char * HUMBLENET_CALL humblenet_get_error() {
    if (humbleNetState.errorString.empty()) {
        return NULL;
    }

    return humbleNetState.errorString.c_str();
}


void HUMBLENET_CALL humblenet_set_error(const char *error) {
    humbleNetState.errorString = error;
}


void HUMBLENET_CALL humblenet_clear_error() {
    humbleNetState.errorString.clear();
}

// END MISC

// BEGIN DEPRECATED

void *humblenet_connection_read_all(Connection *connection, int *retval) {
	assert(connection != NULL);

	if (connection->status == HUMBLENET_CONNECTION_CONNECTING) {
		// not open yet
		return 0;
	}

	if (connection->recvBuffer.empty()) {
		// must not be in poll-returnable connections anymore
		assert(humbleNetState.pendingDataConnections.find(connection) == humbleNetState.pendingDataConnections.end());

		if (connection->status == HUMBLENET_CONNECTION_CLOSED) {
			// TODO: Connection should contain reason it was closed and we should report that
			humblenet_set_error("Connection closed");
			*retval = -1;
			return NULL;
		}

		*retval = 0;
		return NULL;
	}

	size_t dataSize = connection->recvBuffer.size();
	void *buf = malloc(dataSize);

	if (buf == NULL) {
		humblenet_set_error("Memory allocation failed");
		*retval = -1;
		return NULL;
	}

	memcpy(buf, &connection->recvBuffer[0], dataSize);
	connection->recvBuffer.clear();

	auto it = humbleNetState.pendingDataConnections.find(connection);
	assert(it != humbleNetState.pendingDataConnections.end());
	humbleNetState.pendingDataConnections.erase(it);

	*retval = dataSize;
	return buf;
}


Connection *humblenet_poll_all(int timeout_ms) {
	{
		// check for connections closed by remote and notify user
		auto it = humbleNetState.remoteClosedConnections.begin();
		if (it != humbleNetState.remoteClosedConnections.end()) {
			Connection *conn = *it;
			humbleNetState.remoteClosedConnections.erase(it);
			return conn;
		}

		it = humbleNetState.pendingDataConnections.begin();
		if (it != humbleNetState.pendingDataConnections.end()) {
			// don't remove it from the set, _recv will do that
			Connection *conn = *it;
			assert(conn != NULL);
			assert(!conn->recvBuffer.empty());
			return conn;
		}
	}

	// call service if no outstanding data on any connection
	humblenet_poll(timeout_ms);

	{
		auto it = humbleNetState.pendingDataConnections.begin();
		if (it != humbleNetState.pendingDataConnections.end()) {
			// don't remove it from the set, _recv will do that
			Connection *conn = *it;
			assert(conn != NULL);
			assert(!conn->recvBuffer.empty());
			return conn;
		}
	}

	return NULL;
}


ha_bool humblenet_connection_poll(Connection *connection, int timeout_ms) {
	assert(connection != NULL);

    if( !connection->recvBuffer.empty() || (connection->status == HUMBLENET_CONNECTION_CLOSED))
        return true;
    
	// call service so something happens
	humblenet_poll(timeout_ms);

    return (!connection->recvBuffer.empty() || (connection->status == HUMBLENET_CONNECTION_CLOSED));
}


Connection *humblenet_connection_accept() {
	{
		auto it = humbleNetState.pendingNewConnections.begin();

		if (it != humbleNetState.pendingNewConnections.end()) {
			Connection *conn = *it;
			humbleNetState.pendingNewConnections.erase(it);
			return conn;
		}
	}

	// call service so something happens
	// TODO: if we replace poll loop with our own we could poll only the master socket
	humblenet_poll(0);

	{
		auto it = humbleNetState.pendingNewConnections.begin();
		if (it != humbleNetState.pendingNewConnections.end()) {
			Connection *conn = *it;
			humbleNetState.pendingNewConnections.erase(it);
			return conn;
		}
	}

	return NULL;
}

// END DEPRECATED

#ifndef EMSCRIPTEN

#include "libpoll.h"

// Native only API, should be moved to its own file
extern "C" HUMBLENET_API int HUMBLENET_CALL humblenet_select(int nfds, fd_set *readfds, fd_set *writefds,
											  fd_set *exceptfds, struct timeval *timeout) {
    return poll_select( nfds, readfds, writefds, exceptfds, timeout );
}

void HUMBLENET_CALL humblenet_poll(int ms) {
    struct timeval tv;

    // This is not really needed in a threaded environment,
    // e.g. if this is being used as a sleep till something is ready,
    // we need this. If its being used as a "let IO run" (e.g. threaded IO) then we dont.
    if( ms > 0 ) {
        humblenet_lock();
        if( ! humbleNetState.pendingDataConnections.empty() ) {
            ms = 0;
        }
        humblenet_unlock();
    }
            
    tv.tv_sec = ms / 1000;
    tv.tv_usec = 1000 * (ms % 1000);

    poll_select( 0, NULL, NULL, NULL, &tv );
}

void HUMBLENET_CALL humblenet_lock() {
    poll_lock();
}

void HUMBLENET_CALL humblenet_unlock() {
    poll_unlock();
}

#else

void HUMBLENET_CALL humblenet_poll(int ms ) {
}

void HUMBLENET_CALL humblenet_lock() {
}

void HUMBLENET_CALL humblenet_unlock() {
}

#endif


