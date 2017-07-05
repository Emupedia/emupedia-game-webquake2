#include "humblenet_p2p.h"
#include "humblenet_p2p_internal.h"
#include "humblenet_datagram.h"
#include "humblenet_alias.h"

#include <string>
#include <map>

// These are the connections managed by p2p...
std::map<PeerId, Connection*> p2pconnections;

static bool initialized = false;

ha_bool humblenet_p2p_connect_server();

/*
 * Is the peer-to-peer network initialized.
 */
ha_bool HUMBLENET_CALL humblenet_p2p_is_initialized() {
	return initialized;
}

/*
 * Initialize the peer-to-peer library.
 */
ha_bool HUMBLENET_CALL humblenet_p2p_init(const char* server, const char* game_token, const char* game_secret, const char* auth_token) {
	if( initialized ) {
		return 1;
	}

	if (!server || !game_token || !game_secret) {
		humblenet_set_error("Must specify server, game_token, and game_secret");
		return 0;
	}
	initialized = true;
	humbleNetState.signalingServerAddr = server;
	humbleNetState.gameToken = game_token;
	humbleNetState.gameSecret = game_secret;
	humbleNetState.reconnectToken = "";

	if( auth_token ) {
		humbleNetState.authToken = auth_token;
	} else {
		humbleNetState.authToken = "";
	}

	internal_p2p_register_protocol();

	humblenet_p2p_connect_server();

	return 1;
}

/*
 * Shut down the networking library
 */
void HUMBLENET_CALL humblenet_p2p_shutdown() {
    // disconnect from signaling server, shutdown all p2p connections, etc.
    initialized = false;
    humblenet_shutdown();
}

/*
 * Get the current peer ID of this client
 * returns 0 if not yet connected
 */
PeerId HUMBLENET_CALL humblenet_p2p_get_my_peer_id() {
    return humbleNetState.myPeerId;
}

/*
 * Register an alias for this peer so it can be found by name
 */
ha_bool HUMBLENET_CALL humblenet_p2p_register_alias(const char* name) {
	return internal_alias_register( name );
}

/*
 * Unregister an alias of this peer.
 */
ha_bool HUMBLENET_CALL humblenet_p2p_unregister_alias(const char* name) {
	return internal_alias_unregister( name );
}

/*
 * Find the PeerId of a named peer (registered on the server)
 */
PeerId HUMBLENET_CALL humblenet_p2p_virtual_peer_for_alias(const char* name) {
	return internal_alias_lookup( name );
}

/*
 * Send a message to a peer.
 */
int HUMBLENET_CALL humblenet_p2p_sendto(const void* message, uint32_t length, PeerId topeer, uint8_t channel) {
    Connection* conn = NULL;
    
    auto cit = p2pconnections.find( topeer );
    if( cit != p2pconnections.end() ) {
        // we have an active connection
        conn = cit->second;
    } else if( internal_alias_is_virtual_peer( topeer ) ) {
        // lookup/create a connection to the virutal peer.
        conn = internal_alias_find_connection( topeer );
        if( conn == NULL )
            conn = internal_alias_create_connection( topeer );
    } else {
        // create a new connection to the peer
        conn = humblenet_connect_peer( topeer );
    }
    
    if( conn == NULL ) {
        humblenet_set_error("Unable to get a connection for peer");
        return -1;
    } else if( cit == p2pconnections.end() ) {
        p2pconnections.insert( std::make_pair( topeer, conn ) );
        LOG("Connection to peer opened: %u\n", topeer );
    }
    
    int ret = humblenet_datagram_send( message, length, 0, conn, channel );
    TRACE("Sent packet for channel %d to %u(%u): %d\n", channel, topeer, conn->otherPeer, ret );
    if( ret < 0 ) {
        LOG("Peer  %p(%u) write failed: %s\n", conn, topeer, humblenet_get_error() );
        if( humblenet_connection_status( conn ) == HUMBLENET_CONNECTION_CLOSED ) {
            LOG("Peer connection was closed\n");
            humblenet_set_error("Connection to peer was closed");
            return -1;
        }
    }
    return ret;
}

/*
 * See if there is a message available on the specified channel
 */
ha_bool HUMBLENET_CALL humblenet_p2p_select(uint32_t* length, uint8_t channel) {
    Connection* from;
    int ret = humblenet_datagram_recv( NULL, 0, 1/*MSG_PEEK*/, &from, channel );
    if( ret > 0 )
        *length = ret;
    else
        *length = 0;

    return *length > 0;
}

/*
 * Receive a message sent from a peer
 */
int HUMBLENET_CALL humblenet_p2p_recvfrom(void* buffer, uint32_t length, PeerId* frompeer, uint8_t channel) {
    Connection* conn = NULL;
    int ret = humblenet_datagram_recv( buffer, length, 0, &conn, channel );
    if( !conn || !ret ) {
        *frompeer = 0;
    } else {
        PeerId vpeer = internal_alias_get_virtual_peer( conn );
        PeerId peer = conn->otherPeer;
        
        if( vpeer )
            *frompeer = vpeer;
        else
            *frompeer = peer;
        
        if( ret > 0 ) {
            TRACE("Got packet for channel %d from %u(%u)\n", channel, *frompeer, peer );
            if( p2pconnections.find( *frompeer ) == p2pconnections.end() ) {
                LOG("Tracking inbound connection to peer %u(%u)\n", *frompeer, peer );
                p2pconnections.insert( std::make_pair( *frompeer, conn ) );
            }
        } else {
            p2pconnections.erase( *frompeer );
            p2pconnections.erase( peer );
            humblenet_connection_close(conn);
            LOG("closing connection to peer: %u(%u)\n",*frompeer,peer);
        }
    }
    return ret;
}

/*
 * Disconnect a peer
 */
ha_bool HUMBLENET_CALL humblenet_p2p_disconnect(PeerId peer) {
    auto it = p2pconnections.find( peer );
    if( it != p2pconnections.end() )
        humblenet_connection_set_closed( it->second );
    
    // TODO: Should we see if the peer that was passed in is the "real" peer instead of the VPeer?
    return 1;
}


/*
 * Give the p2p system a chance to process whatever it needs to
 */
void HUMBLENET_CALL humblenet_p2p_update() {
    humblenet_poll(0);
}