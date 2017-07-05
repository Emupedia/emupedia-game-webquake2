#include "humblenet_datagram.h"

#include "humblenet_p2p_internal.h"

// TODO : If this had access to the internals of Connection it could be further optimized.

#include <map>
#include <vector>
#include <stdlib.h>
#include <cstring>
#include <stdio.h>

#define HUMBLENET_MSG_PEEK 0x1

struct datagram_connection {
    Connection*         conn;           // established connection.
    PeerId              peer;           // "address"
    bool                outgoing;       // did we initiate the connection? (means we can auto reconnect)

    std::vector<char>   buf_in;         // data we have received buy not yet processed.
    std::vector<char>   buf_out;        // packet combining...
    int                 queued;

    datagram_connection( Connection* conn, bool outgoing )
    :conn( conn )
    ,peer( humblenet_connection_get_peer_id( conn ) )
    ,outgoing( outgoing )
    ,queued( 0 )
    {
    }
};

typedef std::map<Connection*, datagram_connection> ConnectionMap;

static ConnectionMap    connections;
static bool             queuedPackets = false;

static int datagram_get_message( datagram_connection& conn, const void* buffer, size_t length, int flags, uint8_t channel ) {
    if( conn.buf_in.size() <= 3 )
        return 0;

    uint16_t size;
    memcpy(&size, &conn.buf_in[0], 2);
    if ((size + 3) > (uint16_t)conn.buf_in.size()) {
        // incomplete packet
        LOG("Incomplete packet from %u\n", conn.peer );
        return 0;
    }

    if( conn.buf_in[2] != channel ) {
        TRACE("Incorrect channel: wanted %d but got %d(%u) from %u\n", channel, conn.buf_in[2], size, conn.peer );
        return 0;
    }

    if( flags & HUMBLENET_MSG_PEEK )
        return size;

    // TODO: check oversize packet
    memcpy((char*)buffer, &conn.buf_in[3], size);

    conn.buf_in.erase(conn.buf_in.begin(), conn.buf_in.begin() + 3 + size);

    return size;
}

static void datagram_flush( datagram_connection& dg, const char* reason ) {
    if( ! dg.buf_out.empty() )
    {
        if( ! humblenet_connection_is_writable( dg.conn ) ) {
            LOG("Waiting(%s) %d packets (%zu bytes) to  %p\n", reason, dg.queued, dg.buf_out.size(), dg.conn );
            return;
        }

        if( dg.queued > 1 )
            LOG("Flushing(%s) %d packets (%zu bytes) to  %p\n", reason, dg.queued, dg.buf_out.size(), dg.conn );
        int ret = humblenet_connection_write( dg.conn, &dg.buf_out[0], dg.buf_out.size() );
        if( ret < 0 ) {
            LOG("Error flushing packets: %s", humblenet_get_error() );
            humblenet_clear_error();
        }
        dg.buf_out.clear();
        dg.queued = 0;
    }
}

int humblenet_datagram_send( const void* message, size_t length, int flags, Connection* conn, uint8_t channel )
{
    // if were still connecting, we cant write yet
    // TODO: Should we queue that data up?
    switch( humblenet_connection_status( conn ) ) {
        case HUMBLENET_CONNECTION_CONNECTING: {
            humblenet_set_error("Connection is still being establisted");
            return 0;
        }
        case HUMBLENET_CONNECTION_CLOSED: {
            humblenet_set_error("Connection is closed");
            // should this wipe the state ?
            return -1;
        }
        default: {
            break;
        }
    }

    if( true )
    {
        ConnectionMap::iterator it = connections.find( conn );
        if( it == connections.end() ) {
            connections.insert( ConnectionMap::value_type( conn, datagram_connection( conn, false ) ) );
            it = connections.find( conn );
        }

        datagram_connection& dg = it->second;

        dg.buf_out.reserve( dg.buf_out.size() + length + 3 );
        uint16_t len = length;
        char* buf = (char*)&len;
        dg.buf_out.insert( dg.buf_out.end(), buf, buf + 2 );
        dg.buf_out.insert( dg.buf_out.end(), (char)channel );
        buf = (char*)message;
        dg.buf_out.insert( dg.buf_out.end(), buf, buf+len );
        dg.queued++;

        if( dg.buf_out.size() > 1024 ) {
            datagram_flush( dg, "max-length" );;
        } else {
            queuedPackets = true;
            //if( dg.queued > 1 )
            //    LOG("Queued %d packets (%zu bytes) for  %p\n", dg.queued, dg.buf_out.size(), dg.conn );
        }
        return length;
    } else {

    // TODO: Optmize this some, maybe a static buffer etc.
    // build up a packet.
    std::vector<char> datagram( length + 3 );
    uint16_t len = length;
    memcpy(&datagram[0], &len, 2);
    datagram[2] = channel;
    memcpy(&datagram[3], message, length);

    return humblenet_connection_write( conn, &datagram[0], datagram.size() );
    }
}

int humblenet_datagram_recv( void* buffer, size_t length, int flags, Connection** fromconn, uint8_t channel )
{
    // flush queued packets
    if( queuedPackets ) {
        for( ConnectionMap::iterator it = connections.begin(); it != connections.end(); ++it ) {
            datagram_flush( it->second, "auto" );
        }
        queuedPackets = false;
    }

    // first we drain all existing packets.
    for( ConnectionMap::iterator it = connections.begin(); it != connections.end(); ++it ) {
        int ret = datagram_get_message( it->second, buffer, length, flags, channel );
        if( ret ) {
            *fromconn = it->second.conn;
            return ret;
        }
    }

    // next check for incomming data on existing connections...
    while(true) {
        Connection* conn = humblenet_poll_all(0);
        if( conn == NULL )
            break;

        PeerId peer = humblenet_connection_get_peer_id( conn );

        ConnectionMap::iterator it = connections.find( conn );
        if( it == connections.end() ) {
            // hmm connection not cleaned up properly...
            LOG("received data from peer %u, but we have no datagram_connection for them\n", peer );
            connections.insert( ConnectionMap::value_type( conn, datagram_connection( conn, false ) ) );
            it = connections.find( conn );
        }

        // read whatever we can...
        int retval = 0;
        char* buf = (char*)humblenet_connection_read_all(conn, &retval );
        if( retval < 0 ) {
            free( buf );
            connections.erase( it );
            LOG("read from peer %u(%p) failed with %s\n", peer, conn, humblenet_get_error() );
            humblenet_clear_error();
            if( humblenet_connection_status( conn ) == HUMBLENET_CONNECTION_CLOSED ) {
                *fromconn = conn;
                return -1;
            }
            continue;
        } else {
            it->second.buf_in.insert( it->second.buf_in.end(), buf, buf+retval );
            free( buf );
            retval = datagram_get_message( it->second, buffer, length, flags, channel );
            if( retval ) {
                *fromconn = it->second.conn;
                return retval;
            }
        }
    }

    // no existing connections have a packet ready, see if we have any new connections
    while( true ) {
        Connection* conn = humblenet_connection_accept();
        if( conn == NULL )
            break;

        PeerId peer = humblenet_connection_get_peer_id( conn );
        if( peer == 0 ) {
            // Not a peer connection?
            LOG("Accepted connection, but not a peer connection: %p\n", conn);
           // humblenet_connection_close( conn );
            continue;
        }

        connections.insert( ConnectionMap::value_type( conn, datagram_connection( conn, false ) ) );
    }

    return 0;
}

/*
* See if there is a message waiting on the specified channel
*/
ha_bool humblenet_datagram_select( size_t* length, uint8_t channel ) {
    Connection* conn = NULL;
    int ret = humblenet_datagram_recv( NULL, 0, HUMBLENET_MSG_PEEK, &conn, channel );
    if( ret > 0 )
        *length = ret;
    else
        *length = 0;

    return *length > 0;
}

ha_bool humblenet_datagram_flush() {
    // flush queued packets
    if( queuedPackets ) {
        for( ConnectionMap::iterator it = connections.begin(); it != connections.end(); ++it ) {
            datagram_flush( it->second, "manual" );
        }
        queuedPackets = false;
        return true;
    }
    return false;
}
