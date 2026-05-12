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

#include <impl/PlatformSpiDefinition.h>


PlatformSpiDefinition_t BoardSpiDefinitionHatvanLegacy[] = {
    [0] = {
        .peripheral_id = ID_QSPI,
        .baudrate      = 50000000,

        // formatting: before member structs, empty line is needed for correct formatting
        .addr = {
            .peripheral = QSPI,
            .tdr        = (uint32_t)&QSPI->QSPI_TDR,
            .rdr        = (uint32_t)&QSPI->QSPI_RDR,
        },
        .pins = {
            .clk        = PIO_PA14_IDX,
            .clk_flags  = IOPORT_MODE_MUX_A,
            .mosi       = PIO_PA13_IDX,
            .mosi_flags = IOPORT_MODE_MUX_A,
            .miso       = PIO_PA12_IDX,
            .miso_flags = IOPORT_MODE_MUX_A,
            .dio2       = SPI_PIN_NONE,  // not wired on Hatvan board
            .dio2_flags = 0,
            .dio3       = SPI_PIN_NONE,  // not wired on Hatvan board
            .dio3_flags = 0,
            .ls1_dir    = SPI_PIN_NONE,  // not wired on Hatvan board
            .ls2_dir    = SPI_PIN_NONE,  // not wired on Hatvan board
            .csn        = PIO_PA11_IDX,
        },
        .dma = {
            .tx_dma_channel = HV_DMA_HW_CH_TX_2,
            .tx_dma_hw_id   = HV_DMA_HW_INTF_TX_2,
            .rx_dma_channel = HV_DMA_HW_CH_RX_2,
            .rx_dma_hw_id   = HV_DMA_HW_INTF_RX_2,
        },
    },
    [1] = {
        .peripheral_id = ID_SPI1,
        .baudrate      = 50000000,

        // formatting: before member structs, empty line is needed for correct formatting
        .addr = {
            .peripheral = SPI1,
            .tdr        = (uint32_t)&SPI1->SPI_TDR,
            .rdr        = (uint32_t)&SPI1->SPI_RDR,
        },
        .pins = {
            .clk        = PIO_PC24_IDX,
            .clk_flags  = IOPORT_MODE_MUX_C,
            .mosi       = PIO_PC27_IDX,
            .mosi_flags = IOPORT_MODE_MUX_C,
            .miso       = PIO_PC26_IDX,
            .miso_flags = IOPORT_MODE_MUX_C,
            .dio2       = SPI_PIN_NONE,
            .dio2_flags = 0,
            .dio3       = SPI_PIN_NONE,
            .dio3_flags = 0,
            .ls1_dir    = SPI_PIN_NONE,
            .ls2_dir    = SPI_PIN_NONE,
            .csn        = PIO_PC25_IDX,
        },
        .dma = {
            .tx_dma_channel = HV_DMA_HW_CH_TX_1,
            .tx_dma_hw_id   = HV_DMA_HW_INTF_TX_1,
            .rx_dma_channel = HV_DMA_HW_CH_RX_1,
            .rx_dma_hw_id   = HV_DMA_HW_INTF_RX_1,
        },
    },
    [2] = {
        // SPI wing connector using 3.3V
        .peripheral_id = ID_SPI0,
        .baudrate      = 5000000,  // This limit is specific to the FPGA smartar prototype.

        // formatting: before member structs, empty line is needed for correct formatting
        .addr = {
            .peripheral = SPI0,
            .tdr        = (uint32_t)&SPI0->SPI_TDR,
            .rdr        = (uint32_t)&SPI0->SPI_RDR,
        },
        .pins = {
            .clk        = PIO_PD22_IDX,
            .clk_flags  = IOPORT_MODE_MUX_B,
            .mosi       = PIO_PD21_IDX,
            .mosi_flags = IOPORT_MODE_MUX_B,
            .miso       = PIO_PD20_IDX,
            .miso_flags = IOPORT_MODE_MUX_B,
            .dio2       = SPI_PIN_NONE,
            .dio2_flags = 0,
            .dio3       = SPI_PIN_NONE,
            .dio3_flags = 0,
            .ls1_dir    = SPI_PIN_NONE,
            .ls2_dir    = SPI_PIN_NONE,
            .csn        = PIO_PD12_IDX,
        },
        .dma = {
            .tx_dma_channel = HV_DMA_HW_CH_TX_0,
            .tx_dma_hw_id   = HV_DMA_HW_INTF_TX_0,
            .rx_dma_channel = HV_DMA_HW_CH_RX_0,
            .rx_dma_hw_id   = HV_DMA_HW_INTF_RX_0,
        },
    },
};

PlatformSpiDefinition_t BoardSpiDefinitionHatvanPlus[] = {
    [0] = {
        .peripheral_id = ID_QSPI,
        .baudrate      = 50000000,

        // formatting: before member structs, empty line is needed for correct formatting
        .addr = {
            .peripheral = QSPI,
            .tdr        = (uint32_t)&QSPI->QSPI_TDR,
            .rdr        = (uint32_t)&QSPI->QSPI_RDR,
        },
        .pins = {
            .clk        = PIO_PA14_IDX,
            .clk_flags  = IOPORT_MODE_MUX_A,
            .mosi       = PIO_PA13_IDX,
            .mosi_flags = IOPORT_MODE_MUX_A,
            .miso       = PIO_PA12_IDX,
            .miso_flags = IOPORT_MODE_MUX_A,
            .dio2       = PIO_PA17_IDX,
            .dio2_flags = IOPORT_MODE_MUX_A,
            .dio3       = PIO_PD31_IDX,  // also used for RST_N
            .dio3_flags = IOPORT_MODE_MUX_A,
            .ls1_dir    = PIO_PD11_IDX,
            .ls2_dir    = PIO_PD18_IDX,
            .csn        = PIO_PA11_IDX,
        },
        .dma = {
            .tx_dma_channel = HV_DMA_HW_CH_TX_2,
            .tx_dma_hw_id   = HV_DMA_HW_INTF_TX_2,
            .rx_dma_channel = HV_DMA_HW_CH_RX_2,
            .rx_dma_hw_id   = HV_DMA_HW_INTF_RX_2,
        },
    },
    [1] = {
        // SPI wing connector using 3.3V
        .peripheral_id = ID_SPI0,
        .baudrate      = 5000000,  // This limit is specific to the FPGA smartar prototype.

        // formatting: before member structs, empty line is needed for correct formatting
        .addr = {
            .peripheral = SPI0,
            .tdr        = (uint32_t)&SPI0->SPI_TDR,
            .rdr        = (uint32_t)&SPI0->SPI_RDR,
        },
        .pins = {
            .clk        = PIO_PD22_IDX,
            .clk_flags  = IOPORT_MODE_MUX_B,
            .mosi       = PIO_PD21_IDX,
            .mosi_flags = IOPORT_MODE_MUX_B,
            .miso       = PIO_PD20_IDX,
            .miso_flags = IOPORT_MODE_MUX_B,
            .dio2       = SPI_PIN_NONE,
            .dio2_flags = 0,
            .dio3       = SPI_PIN_NONE,
            .dio3_flags = 0,
            .ls1_dir    = SPI_PIN_NONE,
            .ls2_dir    = SPI_PIN_NONE,
            .csn        = PIO_PD12_IDX,
        },
        .dma = {
            .tx_dma_channel = HV_DMA_HW_CH_TX_0,
            .tx_dma_hw_id   = HV_DMA_HW_INTF_TX_0,
            .rx_dma_channel = HV_DMA_HW_CH_RX_0,
            .rx_dma_hw_id   = HV_DMA_HW_INTF_RX_0,
        },
    },
};
