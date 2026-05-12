/**
 * \file 	ProtocolHandler.c
 *
 * \addtogroup      CommunicationInterface      Communication Interface (Command-based)
 * @{
 *   \addtogroup      ProtocolHandler             Protocol Handler
 *   \brief 			Communication protocol handler.
 *   @{
 */
#include "ProtocolHandlerData.h"

#include <stdbool.h>

/******************************************************************************/
/* Type Definitions ----------------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private/Public Variables ---------------------------------------------------*/
/******************************************************************************/
static bool m_sendingDataFrames                          = false;
static uint16_t m_maxPayloadLength                       = 0;
static ProtocolHandlerData_sendDataPacket m_sendFunction = NULL;

static VendorProtocol_DataPacketHeader m_dataPacketHeader = {0, 0, 0, 0};

static uint32_t errorData[][2] = {
    {0, sizeof(uint32_t)},
};

/******************************************************************************/
/*Private/Public Constants ---------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private Methods Declaration ------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private Methods Definition -------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/* Public Methods Definition -----------------------------------------------*/
/******************************************************************************/

void ProtocolHandlerData_setSendFunction(ProtocolHandlerData_sendDataPacket sendFunction, uint16_t maxPacketSize)
{
    m_sendFunction     = sendFunction;
    m_maxPayloadLength = maxPacketSize > sizeof_DataPacketHeader() ? maxPacketSize - sizeof_DataPacketHeader() : 0;

    m_sendingDataFrames = true;
}

sr_t ProtocolHandler_sendDataFrameArray(uint32_t data[][2], uint8_t count, uint8_t channel, uint64_t timestamp)
{
    if (!m_sendFunction || !m_maxPayloadLength)
    {
        return E_NOT_POSSIBLE;
    }

    // check if we have empty data sets at the end
    for (uint_fast8_t i = count - 1u; i > 0; i--)
    {
        const uint32_t length = data[i][1];
        if (length == 0)
        {
            count = i;
        }
        else
        {
            break;
        }
    }

    m_dataPacketHeader.bChannel    = channel;
    const uint32_t timestampLength = timestamp ? sizeof(timestamp) : 0;

    // check if this is a single packet or an error frame
    if ((count < 2) && (((data[0][1] + timestampLength) <= m_maxPayloadLength) || (data[0][0] == 0) || data[0][1] == 0))
    {
        m_dataPacketHeader.bmPktType = DATA_FRAME_SINGLE_PACKET;
        if (timestamp)
        {
            m_dataPacketHeader.bmPktType |= DATA_FRAME_FLAG_TIMESTAMP;
        }

        // check if this is an error frame
        if (data[0][0] == 0)
        {
            m_dataPacketHeader.bmPktType |= DATA_FRAME_FLAG_ERROR;
            errorData[0][0] = (uint32_t)(uintptr_t)&data[0][1];
            data            = errorData;
        }

        // check if this is a debug frame
        if (data[0][1] == 0)
        {
            m_dataPacketHeader.bmPktType |= DATA_FRAME_FLAG_ERROR;
            data[0][1] = strlen((char *)(uintptr_t)data[0][0]);
            if (data[0][1] > m_maxPayloadLength)
            {
                data[0][1] = m_maxPayloadLength;
            }
            if (data[0][1] == sizeof(uint32_t))
            {
                data[0][1] = sizeof(uint32_t) + 1;  // if it has the size of an error code, include the null-termination to distinguish it
            }
            count = 1;  // had been reset to zero by the empty data-set check above
        }
    }
    else
    {
        m_dataPacketHeader.bmPktType = DATA_FRAME_FIRST_PACKET;
    }

    for (uint_fast8_t i = 0; i < count; i++)
    {
        const uint8_t *payload = (const uint8_t *)(uintptr_t)data[i][0];
        uint32_t length        = data[i][1];
        if (i == (count - 1u))
        {
            if (timestamp)
            {
                length += sizeof(timestamp);
            }
        }
        while (length)
        {
            if (!m_sendingDataFrames)
            {
                return E_ABORTED;
            }
            m_dataPacketHeader.wLength = (length > m_maxPayloadLength) ? m_maxPayloadLength : length;
            const sr_t ret             = m_sendFunction(&m_dataPacketHeader, payload, timestamp);
            if (ret != E_SUCCESS)
            {
                m_sendingDataFrames = false;
                return ret;
            }

            //next packet
            m_dataPacketHeader.wCounter++;
            length -= m_dataPacketHeader.wLength;
            payload += m_dataPacketHeader.wLength;
            if ((i == (count - 1u)) && (length <= m_maxPayloadLength))
            {
                m_dataPacketHeader.bmPktType = DATA_FRAME_LAST_PACKET;
                if (timestamp)
                {
                    m_dataPacketHeader.bmPktType |= DATA_FRAME_FLAG_TIMESTAMP;
                }
            }
            else
            {
                m_dataPacketHeader.bmPktType = DATA_FRAME_MIDDLE_PACKET;
            }
        }
    }

    return E_SUCCESS;
}

sr_t ProtocolHandler_sendDataFrame(const uint8_t *payload, uint32_t size, uint8_t channel, uint64_t timestamp)
{
    uint32_t data[][2] = {
        {(uint32_t)(uintptr_t)payload, size},
    };

    return ProtocolHandler_sendDataFrameArray(data, 1, channel, timestamp);
}

sr_t ProtocolHandler_sendErrorFrame(uint32_t code, uint8_t channel, uint64_t timestamp)
{
    uint32_t data[][2] = {
        {0, code},
    };

    return ProtocolHandler_sendDataFrameArray(data, 1, channel, timestamp);
}

sr_t ProtocolHandler_sendDebugFrameImpl(char *message, uint8_t channel, uint64_t timestamp)
{
    uint32_t data[][2] = {
        {(uint32_t)(uintptr_t)message, 0},
    };

    return ProtocolHandler_sendDataFrameArray(data, 1, channel, timestamp);
}


/* @}@} */
