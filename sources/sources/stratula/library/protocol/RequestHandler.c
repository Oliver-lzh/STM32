/**
 * \file 	RequestHandler.c
 *
 * \addtogroup      CommunicationInterface      Communication Interface (Command-based)
 * @{
 *   \addtogroup      RequestHandler              Request Handler
 *   \brief           Interprets and executes commands from remote host.
 *   @{
 */
#include "RequestHandler.h"
#include "CommandHandler.h"
#include <universal/protocol/protocol_definitions.h>

#include "requests/Requests_BoardInfo.h"
#include "requests/Requests_IData.h"
#include "requests/Requests_IGpio.h"
#include "requests/Requests_II2c.h"
#include "requests/Requests_IMemory.h"
#include "requests/Requests_ISpi.h"
#include "requests/Requests_Macro.h"

#include <stddef.h>

/******************************************************************************/
/*Macro Definitions ----------------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private/Public Constants ---------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/*Private/Public Variables ---------------------------------------------------*/
/******************************************************************************/
// instead of having to call the constructor,
// we directly initialize the CommandHandler structs here
static CommandHandler m_commandHandlerComponents = {
    .m_registrationCount = 0,
};
static CommandHandler m_commandHandlerModules = {
    .m_registrationCount = 0,
};
static IRequests *m_requestsMacro  = NULL;
static IRequests *m_requestsCustom = NULL;

/******************************************************************************/
/*Private Methods Declaration ------------------------------------------------*/
/******************************************************************************/

/******************************************************************************/
/* Public Methods Definition -----------------------------------------------*/
/******************************************************************************/
static IGpio *m_gpio;
static ISpi *m_spi;
static IData *m_data;
static II2c *m_i2c;

void RequestHandler_register(IGpio *gpio, ISpi *spi, IData *data, II2c *i2c)
{
    m_gpio = gpio;
    m_spi  = spi;
    m_data = data;
    m_i2c  = i2c;
}

bool RequestHandler_registerComponentImplementation(ICommands *commands)
{
    return CommandHandler_registerImplementation(&m_commandHandlerComponents, commands);
}

bool RequestHandler_registerModuleImplementation(ICommands *commands)
{
    return CommandHandler_registerImplementation(&m_commandHandlerModules, commands);
}

bool RequestHandler_registerMacro(IRequests *requests)
{
    m_requestsMacro = requests;
    return requests != NULL;
}

bool RequestHandler_registerCustom(IRequests *requests)
{
    m_requestsCustom = requests;
    return requests != NULL;
}

uint8_t RequestHandler_write(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, const uint8_t *payload)
{
    switch (bRequest)
    {
        case REQ_BOARD_INFO:
            return Requests_BoardInfo_write(wValue, wIndex, wLength, payload);
            break;
        case REQ_MEMORY:
            return Requests_IMemory_write(wValue, wIndex, wLength, payload);
            break;
        case REQ_GPIO:
            if (m_gpio == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return Requests_IGpio_write(m_gpio, wValue, wIndex, wLength, payload);
            break;
        case REQ_I2C:
            if (m_i2c == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return Requests_II2c_write(m_i2c, wValue, wIndex, wLength, payload);
            break;
        case REQ_I2C_TRANSACTION_16:
            if (m_i2c == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return Requests_II2c_transaction16_write(m_i2c, wValue, wIndex, wLength, payload);
            break;
        case REQ_SPI:
            if (m_spi == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return Requests_ISpi_write(m_spi, wValue, wIndex, wLength, payload);
            break;
        case REQ_DATA:
            if (m_data == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return Requests_IData_write(m_data, wValue, wIndex, wLength, payload);
            break;
        case REQ_MEMORY_STREAM:
            return Requests_IMemory_stream(wValue, wIndex, wLength, payload);
            break;
        case REQ_MACRO:
            if (m_requestsMacro == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return m_requestsMacro->write(wValue, wIndex, wLength, payload);
            break;
        case CMD_COMPONENT:
            return CommandHandler_write(&m_commandHandlerComponents, wValue, wIndex, wLength, payload);
            break;
        case CMD_MODULE:
            return CommandHandler_write(&m_commandHandlerModules, wValue, wIndex, wLength, payload);
            break;
        case REQ_CUSTOM:
            if (m_requestsCustom == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return m_requestsCustom->write(wValue, wIndex, wLength, payload);
            break;
        default:
            break;
    }
    return STATUS_REQUEST_INVALID;
}

uint8_t RequestHandler_read(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLength, uint8_t **payload)
{
    switch (bRequest)
    {
        case REQ_BOARD_INFO:
            return Requests_BoardInfo_read(wValue, wIndex, wLength, payload);
            break;
        case REQ_MEMORY:
            return Requests_IMemory_read(wValue, wIndex, wLength, payload);
            break;
        case REQ_GPIO:
            if (m_gpio == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return Requests_IGpio_read(m_gpio, wValue, wIndex, wLength, payload);
            break;
        case REQ_I2C:
            if (m_i2c == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return Requests_II2c_read(m_i2c, wValue, wIndex, wLength, payload);
            break;
        case REQ_I2C_TRANSACTION_16:
            if (m_i2c == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return Requests_II2c_transaction16_read(m_i2c, wValue, wIndex, wLength, payload);
            break;
        case REQ_SPI:
            if (m_spi == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return Requests_ISpi_read(m_spi, wValue, wIndex, wLength, payload);
            break;
        case REQ_DATA:
            if (m_data == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return Requests_IData_read(m_data, wValue, wIndex, wLength, payload);
            break;
        case REQ_MACRO:
            if (m_requestsMacro == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return m_requestsMacro->read(wValue, wIndex, wLength, payload);
            break;
        case CMD_COMPONENT:
            return CommandHandler_read(&m_commandHandlerComponents, wValue, wIndex, wLength, payload);
            break;
        case CMD_MODULE:
            return CommandHandler_read(&m_commandHandlerModules, wValue, wIndex, wLength, payload);
            break;
        case REQ_CUSTOM:
            if (m_requestsCustom == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return m_requestsCustom->read(wValue, wIndex, wLength, payload);
            break;
        default:
            break;
    }
    return STATUS_REQUEST_INVALID;
}

uint8_t RequestHandler_transfer(uint8_t bRequest, uint16_t wValue, uint16_t wIndex, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut)
{
    switch (bRequest)
    {
        case REQ_BOARD_INFO:
            return Requests_BoardInfo_transfer(wValue, wIndex, wLengthIn, payloadIn, wLengthOut, payloadOut);
            break;
        case REQ_SPI:
            return Requests_ISpi_transfer(m_spi, wValue, wIndex, wLengthIn, payloadIn, wLengthOut, payloadOut);
            break;
        case REQ_MACRO:
            if (m_requestsMacro == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return m_requestsMacro->transfer(wValue, wIndex, wLengthIn, payloadIn, wLengthOut, payloadOut);
            break;
        case CMD_COMPONENT:
            return CommandHandler_transfer(&m_commandHandlerComponents, wValue, wIndex, wLengthIn, payloadIn, wLengthOut, payloadOut);
            break;
        case CMD_MODULE:
            return CommandHandler_transfer(&m_commandHandlerModules, wValue, wIndex, wLengthIn, payloadIn, wLengthOut, payloadOut);
            break;
        case REQ_CUSTOM:
            if (m_requestsCustom == NULL)
            {
                return STATUS_REQUEST_NOT_AVAILABLE;
            }
            return m_requestsCustom->transfer(wValue, wIndex, wLengthIn, payloadIn, wLengthOut, payloadOut);
            break;
        default:
            break;
    }
    return STATUS_REQUEST_INVALID;
}

/* @}@} */
