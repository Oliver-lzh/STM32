#ifndef SOCKETUDPIMPL_H
#define SOCKETUDPIMPL_H 1

#include <common/errors.h>
#include <lwip/pbuf.h>
#include <lwip/udp.h>
#include <stdint.h>


typedef struct udp_pcb *SocketUdp;

/** Function prototype for udp pcb receive callback functions.
* The callback is responsible for freeing the pbuf if it's not used any more.
*
* @param arg user supplied argument
* @param socket the socket which received data
* @param p the packet buffer that was received
* @param addr the remote IP address from which the packet was received
* @param port the remote port from which the packet was received
*/
typedef void (*SocketUdp_recvCb)(void *arg, SocketUdp socket, struct pbuf *p, const ip_addr_t *addr, uint16_t port);


/**
 * Create an Ethernet UDP socket
 */
SocketUdp SocketUdp_createSocket();

/**
 * Open UDP connection
 * @param localPort Port number to use on this side of the connection
 * @param timeout the maximum time to wait for the connection to open
 * @param remoteIp the address to connect to (other side)
 * @param remotePort the port the other side is listening on
 * @return Strata error code: E_FAILED, if applying the configuration fails
 */
sr_t SocketUdp_open(SocketUdp socket, uint16_t localPort, uint16_t timeout, const ip_addr_t *remoteIp, uint16_t remotePort);

/**
 * Close UDP connection
 * @return Strata error code
 */
sr_t SocketUdp_close(SocketUdp socket);

/**
 * Send UDP data to a connected socket
 * @param p the packet buffer to be sent
 * @return Strata error code
 */
sr_t SocketUdp_send(SocketUdp socket, struct pbuf *p);

/**
 * Send UDP data for a connectionless socket, to a specified IP address and port
 *
 * @param p the packet buffer to be sent
 * @param remoteIp destination IP address
 * @param port destination port
 * @return Strata error code
 */
sr_t SocketUdp_sendTo(SocketUdp socket, struct pbuf *p, const ip_addr_t *remoteIp, uint16_t port);

/**
 * Set a receive callback, to be called when a datagram is received by a Udp-socket.
 *
 * @param struct udp_pcb * the socket for which to set the receive callback
 * @param recvCb function pointer of the callback function
 * @param recvArg additional argument to pass to the callback function
 * @return Strata error code
 */
sr_t SocketUdp_setRecvCallback(SocketUdp socket, SocketUdp_recvCb recvCb, void *recvArg);

/**
 * Send an arbitrary buffer to the remote device using UDP connected socket.
 * The write operation completes when:
 * 	- The specified data is written.
 * 	- The time specified by an internal Timeout passes (e.g. TimeConst_10s)
 *
 * @param data a buffer of the specified length
 * @param length number of bytes to be written
 * @return Strata error code
 *
 */
sr_t SocketUdp_write(SocketUdp socket, const uint8_t data[], uint16_t length);

/**
 * Send an arbitrary buffer to the remote device using UDP for a connectionless socket.
 * The write operation completes when:
 *  - The specified data is written.
 *  - The time specified by an internal Timeout passes (e.g. TimeConst_10s)
 *
 * @param remoteIp destination IP address
 * @param port destination port
 * @param data a buffer of the specified length
 * @param length number of bytes to be written
 * @return Strata error code
 *
 */
sr_t SocketUdp_writeTo(SocketUdp socket, ip_addr_t *remoteIp, uint16_t port, const uint8_t data[], uint16_t length);

#endif /* SOCKETUDPIMPL_H */
