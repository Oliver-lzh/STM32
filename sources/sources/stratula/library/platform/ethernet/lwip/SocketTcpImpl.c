#include "SocketTcpImpl.h"
#include <impl/chrono.h>


SocketTcp SocketTcp_createSocket(void)
{
    SocketTcp socket = tcp_new();
    return socket;
}

sr_t SocketTcp_open(SocketTcp *socket, uint16_t localPort)
{
    if (tcp_bind(*socket, IP4_ADDR_ANY, localPort) != ERR_OK)
    {
        return E_FAILED;
    }
    *socket = tcp_listen(*socket);
    if (*socket != NULL)
    {
        return E_SUCCESS;
    }
    else
    {
        return E_FAILED;
    }
}

sr_t SocketTcp_close(SocketTcp socket)
{
    if (socket != NULL)
    {
        tcp_close(socket);
    }
    return E_SUCCESS;
}

void SocketTcp_setNagle(SocketTcp socket, bool enable)
{
    if (enable)
    {
        tcp_nagle_enable(socket);
    }
    else
    {
        tcp_nagle_disable(socket);
    }
}

sr_t SocketTcp_setAcceptCallback(SocketTcp socket, SocketTcp_acceptCb acceptCb, void *arg)
{
    tcp_arg(socket, arg);
    tcp_accept(socket, acceptCb);
    return E_SUCCESS;
}

sr_t SocketTcp_setReceiveCallback(SocketTcp socket, SocketTcp_recvCb recvCb, void *arg)
{
    tcp_arg(socket, arg);
    tcp_recv(socket, recvCb);
    return E_SUCCESS;
}

sr_t SocketTcp_confirmReceived(SocketTcp socket, uint16_t byteCnt)
{
    tcp_recved(socket, byteCnt);
    return E_SUCCESS;
}

sr_t SocketTcp_write(SocketTcp socket, const uint8_t *data, uint16_t length, bool moreData)
{
    err_t err = tcp_write(socket, data, length, TCP_WRITE_FLAG_COPY);
    if (err == ERR_OK)
    {
        if (!moreData)
        {
            tcp_output(socket);
        }
        return E_SUCCESS;
    }
    else
    {
        return E_FAILED;
    }
}
