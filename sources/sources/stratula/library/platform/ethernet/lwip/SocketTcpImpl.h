#ifndef SOCKETTCPIMPL_H
#define SOCKETTCPIMPL_H 1

#include <common/errors.h>
#include <lwip/tcp.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct tcp_pcb *SocketTcp;

/** Function prototype for tcp pcb accept callback functions.
* The function is called when a new client connection was established.
*
* @param arg user supplied argument
* @param newSocket the new socket of the new connection
* @param err an error code if there has been an error accepting.
* @return Error code of the callback. Only return ERR_ABRT if you have called tcp_abort from within the callback function!
*/
typedef err_t (*SocketTcp_acceptCb)(void *arg, SocketTcp newSocket, err_t err);

/** Function prototype for receive callback functions.
* The function is called when data is received on a socket or the connection was closed.
* The callback is responsible for freeing the pbuf if it's not used anymore.
*
* @param arg user supplied argument
* @param socket the socket which received data
* @param p the packet buffer that was received or NULL if the connection was closed
* @param err an error code if there has been an error accepting.
* @return Error code of the callback. Only return ERR_ABRT if you have called tcp_abort from within the callback function!
*/
typedef err_t (*SocketTcp_recvCb)(void *arg, SocketTcp socket, struct pbuf *p, err_t err);


/**
 * Create an Ethernet TCP socket
 */
SocketTcp SocketTcp_createSocket();

/**
 * Open TCP connection
 * @param socket The socket to open for connections.
 * @param localPort Port number to use on this side of the connection
 * @return Strata error code: E_FAILED, if applying the configuration fails
 * @note The socket is provided as pointer, because it has to be reallocated for this process.
 */
sr_t SocketTcp_open(SocketTcp *socket, uint16_t localPort);

/**
 * Close TCP connection
 * @param socket The socket, whose connection shall be closed
 * @return Strata error code
 */
sr_t SocketTcp_close(SocketTcp socket);

/**
 * Enable / disable the usage of Nagle Algorithm
 * The Nagle algorithm delays outgoing packets smaller than the maximum segment size
 * as long as the previous packet is not acknowledged. It is enabled if the function is never called.
 * @note On internet connections this can be beneficial to avoid lots of small packets.
 *       On local connections it can have negative impact due to unwanted delays.
 * @note This function must be called on connection sockets, not accept sockets.
 * @param socket The socket the setting is for
 * @param enable True to enable the Nagle Algorithm, false to disable
 */
void SocketTcp_setNagle(SocketTcp socket, bool enable);

/**
 * Set an accept callback, to be called when a new connection was established.
 *
 * @param socket the socket for which to set the accept callback
 * @param acceptCb function pointer of the callback function
 * @param arg additional argument to pass to the callback function, whenever it is called
 * @return Strata error code
 */
sr_t SocketTcp_setAcceptCallback(SocketTcp socket, SocketTcp_acceptCb acceptCb, void *arg);

/**
 * Set a receive callback, to be called when a datagram is received by the socket.
 * @note Whenever the receive callback function is called you have to call SocketTcp_confirmReceived.
 *
 * @param socket the socket for which to set the receive callback
 * @param recvCb function pointer of the callback function
 * @param arg additional argument to pass to the callback function
 * @return Strata error code
 */
sr_t SocketTcp_setReceiveCallback(SocketTcp socket, SocketTcp_recvCb recvCb, void *arg);

/**
 * Call this function to signal that you properly received some data.
 * @note Not calling this function will decrease the receive window resulting in blocked communication
 * @param socket the socket on which the processed data was received
 * @param byteCnt The number of bytes that have been processed. Can e.g. be the tot_len of the last received pbuf
 * @return Strata error code
 */
sr_t SocketTcp_confirmReceived(SocketTcp socket, uint16_t byteCnt);

/**
 * Send an arbitrary buffer to the remote device using TCP connected socket.
 * @param socket the socket which to write the data to
 * @param data a buffer with data to send
 * @param length number of bytes to be written
 * @param moreData Set to true if there will be more data which can be combined in one transmission, false to send immediately
 * @return Strata error code
 *
 */
sr_t SocketTcp_write(SocketTcp socket, const uint8_t *data, uint16_t length, bool moreData);

#endif /* SOCKETTCPIMPL_H */
