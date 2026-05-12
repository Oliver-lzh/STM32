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

/**
 * \addtogroup      SpiPlatform
 * @{
 */
#ifndef PLATFORM_SPI_DEFINITION_H
#define PLATFORM_SPI_DEFINITION_H 1

#include <ioport.h>
#include <stdint.h>
#include <xdmac.h>

#define SPI_PIN_NONE (0xFFFFFFFF)

#define HV_DMA_HW_CH_TX_0   0 /* XMDAC channel for TX channel 0 */
#define HV_DMA_HW_CH_RX_0   1 /* XMDAC channel for RX channel 0 */
#define HV_DMA_HW_CH_TX_1   2 /* XMDAC channel for TX channel 1 */
#define HV_DMA_HW_CH_RX_1   3 /* XMDAC channel for RX channel 1 */
#define HV_DMA_HW_CH_TX_2   4 /* XMDAC channel for TX channel 2 */
#define HV_DMA_HW_CH_RX_2   5 /* XMDAC channel for RX channel 2 */
#define HV_DMA_HW_INTF_TX_0 1 /* XDMAC channel HW interface for SPI0 TX, refer to XDMAC_CC.PERID in datasheet */
#define HV_DMA_HW_INTF_RX_0 2 /* XDMAC channel HW interface for SPI0 RX, refer to XDMAC_CC.PERID in datasheet */
#define HV_DMA_HW_INTF_TX_1 3 /* XDMAC channel HW interface for SPI1 TX, refer to XDMAC_CC.PERID in datasheet */
#define HV_DMA_HW_INTF_RX_1 4 /* XDMAC channel HW interface for SPI1 RX, refer to XDMAC_CC.PERID in datasheet */
#define HV_DMA_HW_INTF_TX_2 5 /* XDMAC channel HW interface for QSPI TX, refer to XDMAC_CC.PERID in datasheet */
#define HV_DMA_HW_INTF_RX_2 6 /* XDMAC channel HW interface for QSPI RX, refer to XDMAC_CC.PERID in datasheet */

typedef struct _PlatformSpiDefinition_t PlatformSpiDefinition_t;

struct _PlatformSpiDefinition_t
{
    uint32_t peripheral_id;  ///< ID of underlying HW peripheral
    uint32_t baudrate;       ///< SPI max baud rate

    struct
    {
        ioport_pin_t clk;          ///< master clock I/O pin
        ioport_mode_t clk_flags;   ///< master clock I/O pin configuration flags
        ioport_pin_t mosi;         ///< master MOSI pin (QSPI DIO0)
        ioport_mode_t mosi_flags;  ///< master MOSI pin configuration flags
        ioport_pin_t miso;         ///< master MISO pin (QSPI DIO1)
        ioport_mode_t miso_flags;  ///< master MISO pin configuration flags
        ioport_pin_t dio2;         ///< master DIO2 pin (QSPI)
        ioport_mode_t dio2_flags;  ///< master DIO2 pin configuration flags
        ioport_pin_t dio3;         ///< master DIO3 pin (QSPI)
        ioport_mode_t dio3_flags;  ///< master DIO3 pin configuration flags
        ioport_pin_t ls1_dir;      ///< controls the level shifter direction of DIO1_MISO
        ioport_pin_t ls2_dir;      ///< controls the level shifter direction of DIO0_MOSI, DIO2 and DIO3_RSTN
        ioport_pin_t csn;          ///< device slave select pin
    } pins;

    struct
    {
        /** \brief lock used to prevent concurrent access conflicts to the SPI interface between the main loop
        * and the interrupt for the BGT IRQ line which also starts SPI transfers  */
        volatile uint32_t lock;
    } state;

    /** \brief register addresses used by this SPI interface */
    struct
    {
        void *peripheral;  ///< Pointer to underlying HW peripheral
        /** \brief Address of the SPI RDR peripheral register */
        uint32_t rdr;
        /** \brief Address of the SPI TDR peripheral register */
        uint32_t tdr;
    } addr;

    /** \brief state to be kept between SPI IRQ and DMA transfer complete IRQ */
    volatile struct
    {
        uint8_t tx_dma_channel;  ///< SPI tx dma channel
        uint8_t tx_dma_hw_id;    ///< SPI tx dma hardware interface ID
        uint8_t rx_dma_channel;  ///< SPI rx dma channel
        uint8_t rx_dma_hw_id;    ///< SPI rx dma hardware interface ID
        /** \brief  DMA structure for SPI TX transfer (can be expanded as a linked list)*/
        COMPILER_WORD_ALIGNED lld_view2 dma_tx;
        /** \brief  DMA structure for SPI RX transfer (can be expanded as a linked list)*/
        COMPILER_WORD_ALIGNED lld_view2 dma_rx;
    } dma;

    /** \brief structure containing the transfer complete callback and associated data */
    volatile struct
    {
        /** \brief function to be called when transfer is complete */
        void (*fn)(void *);
        /** \brief context for callback */
        void *context;
    } callback;
};

#endif /* PLATFORM_SPI_DEFINITION_H */

/** @} */
