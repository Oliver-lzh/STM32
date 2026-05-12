#ifndef ETHERNET_IMPL_H
#define ETHERNET_IMPL_H 1

#include <common/errors.h>
#include <stdint.h>


/**
 * Initialize Ethernet
 */
void Ethernet_Constructor(const uint8_t macAddr[6]);

/**
 * Set IP address if DHCP or AUTO_IP is not used
 */
void Ethernet_setIp(const uint8_t ipAddr[4], const uint8_t netmask[4], const uint8_t gateway[4]);

/**
 *   This function shall be periodically called in the application main-loop,
 *   in order to check the system's state and check for incoming data
 *
 */
void Ethernet_run(void);


#endif /* ETHERNET_IMPL_H */
