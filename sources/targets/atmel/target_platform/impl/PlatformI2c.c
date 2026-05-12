/* ===========================================================================
** Copyright (C) 2021 Infineon Technologies AG
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice,
**    this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
** ===========================================================================
*/

#include "PlatformI2c.h"
#include "SamsHelper.h"

#include <impl/chrono.h>
#include <twihs.h>

#include <impl/thread.h>

/******************************************************************************/
/*Macro Definitions ----------------------------------------------------------*/
/******************************************************************************/
// Maximum number of bytes per i2c-transfer
#define PLATFORM_I2C_MAX_TRANSFER_SIZE UINT16_MAX
// i2c Ack Polling Timeout(ms)
#define PLATFORM_I2C_TRANSFER_TIMEOUT 100
/******************************************************************************/
/*Private/Public Variables ---------------------------------------------------*/
/******************************************************************************/
static uint8_t m_busCount = 0;
static const PlatformI2cDefinition_t *m_busConfig;
static uint32_t m_mck;

/******************************************************************************/
/*Local Function Prototypes  -------------------------------------------------*/
/******************************************************************************/

/**
 * Translates Atmel Error into Strata Error Definition.
 *
 * @param	status  uint32_t with Atmel definition
 * @return	Strata error code
 */
static inline sr_t translateError(uint32_t status);

/**
 * Configures i2c bus sda and scl pins in GPIO mode or peripheral mode.
 *
 * @param bus		PlatformI2cDefinition_t i2c bus pointer
 * @param enable	bool					true -> configures pin as Peripheral, false -> configure pins as GPIO (disables I2c function)
 * @return none
 */
static void configureBusPins(const PlatformI2cDefinition_t *bus, bool enable);

/**
 * write implementation for i2c.
 *
 * @param  devAddr	uint16_t contains device address
 * @param  length	uint16_t packet length
 * @param  packet   twihs_packet_t* pointer to atmel packet structure
 * @param  buffer   uint8_t[] pointer to write buffer
 * @return Strata error code
 */
static sr_t writeImpl(uint16_t devAddr, uint16_t length, twihs_packet_t *packet, const uint8_t buffer[]);

/**
 * read implementation for i2c.
 *
 * @param  devAddr	uint16_t contains device address
 * @param  length	uint16_t packet length
 * @param  packet   twihs_packet_t* pointer to atmel packet structure
 * @param  buffer   uint8_t[] pointer to read buffer
 * @return Strata error code
 */
static sr_t readImpl(uint16_t devAddr, uint16_t length, twihs_packet_t *packet, uint8_t buffer[]);

/******************************************************************************/
/*Local Function Definitions -------------------------------------------------*/
/******************************************************************************/
static inline sr_t translateError(uint32_t status)
{
    switch (status)
    {
        case TWIHS_SUCCESS:
            return E_SUCCESS;
            break;
        case TWIHS_ERROR_TIMEOUT:
            return E_TIMEOUT;
            break;
        case TWIHS_RECEIVE_NACK:
            return E_ABORTED;
            break;
        case TWIHS_ARBITRATION_LOST:
            return E_BUSY;
            break;
        case TWIHS_INVALID_ARGUMENT:
            return E_INVALID_PARAMETER;
            break;
        default:
            return E_FAILED;
            break;
    }
}

static void configureBusPins(const PlatformI2cDefinition_t *bus, bool enable)
{
    if (enable)
    {
        // configure bus->scl and bus->sda into I2C Peripheral mode
        ioport_set_pin_mode(bus->scl, bus->scl_flags);
        ioport_disable_pin(bus->scl);

        ioport_set_pin_mode(bus->sda, bus->sda_flags);
        ioport_disable_pin(bus->sda);
    }
    else
    {
        // configure bus->scl and bus->sda into GPIO mode
        ioport_set_pin_mode(bus->scl, IOPORT_MODE_PULLDOWN);
        ioport_enable_pin(bus->scl);

        ioport_set_pin_mode(bus->sda, IOPORT_MODE_PULLDOWN);
        ioport_enable_pin(bus->sda);
    }
}

static sr_t writeImpl(uint16_t devAddr, uint16_t length, twihs_packet_t *packet, const uint8_t buffer[])
{
    const uint8_t busId = GET_I2C_BUS_ID_VALUE(devAddr);
    uint8_t addr        = (uint8_t)devAddr;

    if (length > PLATFORM_I2C_MAX_TRANSFER_SIZE)
    {
        return E_INVALID_SIZE;
    }
    if (busId >= m_busCount)
    {
        return E_NOT_INITIALIZED;
    }

    const PlatformI2cDefinition_t *bus = &m_busConfig[busId];

    packet->chip   = addr;
    packet->buffer = (void *)buffer;
    packet->length = length;

    const uint32_t status = twihs_master_write(bus->twihs, packet);

    return translateError(status);
}

static sr_t readImpl(uint16_t devAddr, uint16_t length, twihs_packet_t *packet, uint8_t buffer[])
{
    const uint8_t busId = GET_I2C_BUS_ID_VALUE(devAddr);
    uint8_t addr        = (uint8_t)devAddr;

    if (length > PLATFORM_I2C_MAX_TRANSFER_SIZE)
    {
        return E_INVALID_SIZE;
    }
    if (busId >= m_busCount)
    {
        return E_NOT_INITIALIZED;
    }
    const PlatformI2cDefinition_t *bus = &m_busConfig[busId];

    packet->chip   = addr;
    packet->buffer = buffer;
    packet->length = length;

    const uint32_t status = twihs_master_read(bus->twihs, packet);

    return translateError(status);
}

/******************************************************************************/
/*Interface Methods Definition -----------------------------------------------*/
/******************************************************************************/

sr_t PlatformI2c_writeWithoutPrefix(uint16_t devAddr, uint16_t length, const uint8_t buffer[])
{
    /* Configure the data packet to be received */
    twihs_packet_t packet;
    packet.addr_length = 0;

    return writeImpl(devAddr, length, &packet, buffer);
}

sr_t PlatformI2c_writeWith8BitPrefix(uint16_t devAddr, uint8_t prefix, uint16_t length, const uint8_t buffer[])
{
    /* Configure the data packet to be received */
    twihs_packet_t packet;
    packet.addr[0]     = prefix;
    packet.addr_length = sizeof(prefix);

    return writeImpl(devAddr, length, &packet, buffer);
}

sr_t PlatformI2c_writeWith16BitPrefix(uint16_t devAddr, uint16_t prefix, uint16_t length, const uint8_t buffer[])
{
    /* Configure the data packet to be received */
    twihs_packet_t packet;
    packet.addr[0]     = (uint8_t)(prefix >> 8u);
    packet.addr[1]     = (uint8_t)(prefix);
    packet.addr_length = sizeof(prefix);

    return writeImpl(devAddr, length, &packet, buffer);
}

sr_t PlatformI2c_readWithoutPrefix(uint16_t devAddr, uint16_t length, uint8_t buffer[])
{
    twihs_packet_t packet;
    packet.addr_length = 0;

    return readImpl(devAddr, length, &packet, buffer);
}

sr_t PlatformI2c_readWith8BitPrefix(uint16_t devAddr, uint8_t prefix, uint16_t length, uint8_t buffer[])
{
    twihs_packet_t packet;
    packet.addr[0]     = prefix;
    packet.addr_length = sizeof(prefix);

    return readImpl(devAddr, length, &packet, buffer);
}

sr_t PlatformI2c_readWith16BitPrefix(uint16_t devAddr, uint16_t prefix, uint16_t length, uint8_t buffer[])
{
    twihs_packet_t packet;
    packet.addr[0]     = (uint8_t)(prefix >> 8u);
    packet.addr[1]     = (uint8_t)(prefix);
    packet.addr_length = sizeof(prefix);

    return readImpl(devAddr, length, &packet, buffer);
}

uint32_t PlatformI2c_getMaxTransfer(void)
{
    return PLATFORM_I2C_MAX_TRANSFER_SIZE;
}

sr_t PlatformI2c_configureBusSpeed(uint16_t devAddr, uint32_t speed)
{
    const uint8_t busId = GET_I2C_BUS_ID_VALUE(devAddr);

    if (busId >= m_busCount)
    {
        return E_OUT_OF_BOUNDS;
    }
    const PlatformI2cDefinition_t *bus = &m_busConfig[busId];
    if (twihs_set_speed(bus->twihs, speed, m_mck) != PASS)
    {
        return E_FAILED;
    }
    return E_SUCCESS;
}

sr_t PlatformI2c_pollForAck(uint16_t devAddr)
{
    const uint8_t busId = GET_I2C_BUS_ID_VALUE(devAddr);
    uint8_t addr        = (uint8_t)devAddr;

    if (busId >= m_busCount)
    {
        return E_NOT_INITIALIZED;
    }

    const PlatformI2cDefinition_t *bus = &m_busConfig[busId];
    uint32_t status;

    /* I2C ACK polling */
    const chrono_ticks_t deadline = chrono_get_timepoint(chrono_milliseconds(PLATFORM_I2C_TRANSFER_TIMEOUT));
    do
    {
        status = twihs_probe(bus->twihs, addr);
        if (status != TWIHS_RECEIVE_NACK)
        {
            break;
        }
    } while (!chrono_has_passed(deadline));

    return translateError(status);
}

sr_t PlatformI2c_clearBus(uint16_t devAddr)
{
    const uint8_t busId = GET_I2C_BUS_ID_VALUE(devAddr);
    if (busId >= m_busCount)
    {
        return E_OUT_OF_BOUNDS;
    }
    const PlatformI2cDefinition_t *bus = &m_busConfig[busId];

    // for compatibility between Silicon Revisions: ATSAMS70B and ATSAMS70A
    if (Sams70RevisionA())
    {
        // manually issue a clear bus command in case of Revision A due to faulty HW module implementation
        // (recommended workaround in Errata sheet)
        ioport_set_pin_mode(bus->scl, IOPORT_MODE_OPEN_DRAIN);
        ioport_set_pin_dir(bus->scl, IOPORT_DIR_OUTPUT);
        ioport_set_pin_level(bus->scl, IOPORT_PIN_LEVEL_HIGH);
        ioport_enable_pin(bus->scl);
        for (uint8_t i = 0; i < 9; i++)
        {
            ioport_set_pin_level(bus->scl, IOPORT_PIN_LEVEL_LOW);
            this_thread_sleep_for(chrono_microseconds(200));
            ioport_set_pin_level(bus->scl, IOPORT_PIN_LEVEL_HIGH);
            this_thread_sleep_for(chrono_microseconds(200));
        }
        ioport_set_pin_dir(bus->scl, IOPORT_DIR_INPUT);
        configureBusPins(bus, true);
    }
    else
    {
        // issue a bus clear command
        bus->twihs->TWIHS_CR = TWIHS_CR_CLEAR;
    }

    return E_SUCCESS;
}

sr_t PlatformI2c_enableBus(uint16_t devAddr, bool enable)
{
    const uint8_t busId = GET_I2C_BUS_ID_VALUE(devAddr);
    if (busId >= m_busCount)
    {
        return E_OUT_OF_BOUNDS;
    }
    const PlatformI2cDefinition_t *bus = &m_busConfig[busId];

    configureBusPins(bus, enable);
    return E_SUCCESS;
}

sr_t PlatformI2c_readBus(uint16_t devAddr, uint8_t *levels)
{
    const uint8_t busId = GET_I2C_BUS_ID_VALUE(devAddr);
    if (busId >= m_busCount)
    {
        return E_OUT_OF_BOUNDS;
    }

    const PlatformI2cDefinition_t *bus = &m_busConfig[busId];

    *levels = 0;
    if (ioport_get_pin_level(bus->sda))
    {
        *levels |= (1u << 0);
    }
    if (ioport_get_pin_level(bus->scl))
    {
        *levels |= (1u << 1);
    }

    return E_SUCCESS;
}

sr_t PlatformI2c_initialize(const PlatformI2cDefinition_t *definition, uint8_t busCount)
{
    m_busConfig = definition;
    m_busCount  = busCount;

    twihs_options_t opt;
    m_mck          = sysclk_get_peripheral_hz();
    opt.master_clk = m_mck;
    for (uint8_t busId = 0; busId < busCount; busId++)
    {
        pmc_enable_periph_clk(definition[busId].id_twihs);
        opt.speed             = definition[busId].speed;
        const uint32_t status = twihs_master_init(definition[busId].twihs, &opt);
        if (status != TWIHS_SUCCESS)
        {
            return E_FAILED;
        }
        // configure bus->scl and bus->sda to Peripheral mode
        configureBusPins(&definition[busId], true);
    }
    return E_SUCCESS;
}
