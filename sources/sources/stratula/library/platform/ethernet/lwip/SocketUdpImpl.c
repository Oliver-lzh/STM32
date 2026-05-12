#include "SocketUdpImpl.h"
#include <impl/chrono.h>


SocketUdp SocketUdp_createSocket(void)
{
    SocketUdp socket = udp_new();
    return socket;
}

sr_t SocketUdp_open(SocketUdp socket, uint16_t localPort, uint16_t timeout, const ip_addr_t *remoteIp, uint16_t remotePort)
{
    if (udp_bind(socket, IP4_ADDR_ANY, localPort) != ERR_OK)
    {
        return E_FAILED;
    }
    if ((remoteIp != NULL) && (remotePort != 0))
    {
        if (udp_connect(socket, remoteIp, remotePort) != ERR_OK)
        {
            return E_FAILED;
        }
    }
    return E_SUCCESS;
}

sr_t SocketUdp_close(SocketUdp socket)
{
    udp_disconnect(socket);
    return E_SUCCESS;
}

sr_t SocketUdp_send(SocketUdp socket, struct pbuf *p)
{
    if (udp_send(socket, p) != ERR_OK)
    {
        return E_FAILED;
    }
    return E_SUCCESS;
}

sr_t SocketUdp_sendTo(SocketUdp socket, struct pbuf *p, const ip_addr_t *remoteIp, uint16_t port)
{
    if (udp_sendto(socket, p, remoteIp, port) != ERR_OK)
    {
        return E_FAILED;
    }
    return E_SUCCESS;
}

sr_t SocketUdp_setRecvCallback(SocketUdp socket, SocketUdp_recvCb recvCb, void *recvArg)
{
    udp_recv(socket, recvCb, recvArg);
    return E_SUCCESS;
}

sr_t SocketUdp_write(SocketUdp socket, const uint8_t data[], uint16_t length)
{
    struct pbuf p = {
        .type_internal = PBUF_REF,
        .len           = length,
        .tot_len       = length,
        .payload       = (void *)data,
        .next          = NULL,
        .ref           = 1,
        .flags         = 0,
    };

    const chrono_ticks_t timeout = chrono_now() + chrono_milliseconds(1);
    do
    {
        if (udp_send(socket, &p) == ERR_OK)
        {
            return E_SUCCESS;
        }
    } while (chrono_now() < timeout);
    return E_TIMEOUT;
}

sr_t SocketUdp_writeTo(SocketUdp socket, ip_addr_t *remoteIp, uint16_t port, const uint8_t data[], uint16_t length)
{
    struct pbuf p = {
        .type_internal = PBUF_REF,
        .len           = length,
        .tot_len       = length,
        .payload       = (void *)data,
        .next          = NULL,
        .ref           = 1,
        .flags         = 0,
    };

    chrono_ticks_t timeout = chrono_now() + chrono_milliseconds(1);
    do
    {
        if (udp_sendto(socket, &p, remoteIp, port) == ERR_OK)
        {
            return E_SUCCESS;
        }
    } while (chrono_now() < timeout);
    return E_TIMEOUT;
}
