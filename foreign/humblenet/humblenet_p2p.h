#ifndef HUMBLENET_P2P_H
#define HUMBLENET_P2P_H

#include "humblenet.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
* Is the peer-to-peer network supported on this platform.
*/
HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_p2p_supported();
	
/*
* Initialize the peer-to-peer library.
*/
HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_p2p_init(const char* server, const char* client_token, const char* client_secret, const char* auth_token);

/*
* Is the peer-to-peer network supported on this platform.
*/
HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_p2p_is_initialized();
	
/*
* Shut down the networking library
*/
HUMBLENET_API void HUMBLENET_CALL humblenet_p2p_shutdown();

/*
* Get the current peer ID of this client
* returns 0 if not yet connected
*/
HUMBLENET_API PeerId HUMBLENET_CALL humblenet_p2p_get_my_peer_id();
	
/*
* Register an alias for this peer so it can be found by name
*/
HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_p2p_register_alias(const char* name);

/*
 * Unregister an alias for this peer. Passing NULL will unpublish all aliases for the peer.
 */
HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_p2p_unregister_alias(const char* name);

/*
 * Create a virtual peer for an alias on the server
 */
HUMBLENET_API PeerId HUMBLENET_CALL humblenet_p2p_virtual_peer_for_alias(const char* name);

/*
* Send a message to a peer.
*/
HUMBLENET_API int HUMBLENET_CALL humblenet_p2p_sendto(const void* message, uint32_t length, PeerId topeer, uint8_t nChannel);

/*
* Test if a message is available on the specified channel. 
*/
HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_p2p_select(uint32_t* length, uint8_t nChannel);
	
/*
* Receive a message sent from a peer
*/
HUMBLENET_API int HUMBLENET_CALL humblenet_p2p_recvfrom(void* buffer, uint32_t length, PeerId* frompeer, uint8_t nChannel);

/*
* Disconnect a peer
*/
HUMBLENET_API ha_bool HUMBLENET_CALL humblenet_p2p_disconnect(PeerId peer);
	
/*
* Give the p2p system a chance to process whatever it needs to
*/
HUMBLENET_API void HUMBLENET_CALL humblenet_p2p_update();
	

#ifdef __cplusplus
}
#endif

#endif /* HUMBLENET_P2P_H */
