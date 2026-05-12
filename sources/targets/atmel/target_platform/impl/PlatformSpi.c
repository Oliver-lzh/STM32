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

#include "PlatformSpi.h"

#include "ConfigurationIsr.h"

/* Here we include qspi.h and spi.h directly without going through asf.h as this avoids some duplicate definitions */
#include <qspi.h>
#include <spi.h>

/******************************************************************************/
/*Macro Definitions ----------------------------------------------------------*/
/******************************************************************************/
#ifndef PLATFORM_SPI_MAX_DEVICES
#define PLATFORM_SPI_MAX_DEVICES 4u
#endif

// Maximum number of bits per Spi-transfer
#define PlatformSpi_MIN_DATA_WIDTH 2u
#define PlatformSpi_MAX_DATA_WIDTH 32u
#define PlatformSpi_MAX_TRANSFER   32768u
//#define PlatformSpi_DATA_TRANSFER_TIMEOUT      TimeConst_100ms // TODO: implement timeouts within transfer functions

// SPI/QSPI Macros Definition
#define MIN_DELAY_QCS         0
#define TXDELAY               0  // 0x40
#define SPI_NBITS_CALC_SHIFT  4  // To calculate number of bits from word size
#define QSPI_NBITS_CALC_SHIFT 8  // To calculate number of bits from word size

/******************************************************************************/
/*Private/Public Variables ---------------------------------------------------*/
/******************************************************************************/

static PlatformSpiDefinition_t *m_definition;
static uint8_t m_count                                   = 0;
static volatile uint8_t m_devIdIsr                       = PLATFORM_SPI_MAX_DEVICES;
static bool m_deviceConfigured[PLATFORM_SPI_MAX_DEVICES] = {false};

static const bool m_csnIdleLevel              = true;  // TODO: make device specific: slave select idle level (true=high, false=low)
static const uint32_t m_qspiReadoutWaitCycles = 2;     // TODO: make device specific

/******************************************************************************/
/* Function Prototypes ------------------------------------------------------*/
/******************************************************************************/

static sr_t _transfer8(PlatformSpiDefinition_t *self, const uint8_t *txd, uint8_t *rxd, uint32_t count);
static sr_t _transfer16(PlatformSpiDefinition_t *self, const uint16_t *txd, uint16_t *rxd, uint32_t count);
static void drop_rx(PlatformSpiDefinition_t *self);
static void set_word_size(PlatformSpiDefinition_t *self, uint8_t wordSize);
static void startQspiInstruction(PlatformSpiDefinition_t *self, uint32_t waitCycles, uint8_t code, bool enableCode, bool enableData, bool readInstruction, uint8_t wordSize);

/******************************************************************************/
/* Function Definitions ------------------------------------------------------*/
/******************************************************************************/

static inline enum qspi_run_mode getQspiMode(PlatformSpiDefinition_t *self)
{
    Qspi *qspi = (Qspi *)self->addr.peripheral;
    return qspi->QSPI_MR & QSPI_MR_SMM;
}

static inline bool isQspiConfigured(PlatformSpiDefinition_t *self)
{
    if (self->peripheral_id == ID_QSPI)
    {
        return getQspiMode(self) == QSPI_MR_SMM_MEMORY;
    }
    return false;
}

static inline void waitQspiInstructionFinish(Qspi *qspi)
{
    while (!(qspi->QSPI_SR & QSPI_SR_INSTRE))  // wait for instruction ready flag
        ;
}

static inline void finishQspiReadout(PlatformSpiDefinition_t *self)
{
    Qspi *qspi    = (Qspi *)self->addr.peripheral;
    qspi->QSPI_CR = QSPI_CR_LASTXFER;  // inform peripheral that last word will be read
    waitQspiInstructionFinish(qspi);
}

static inline void configurePeripheralPin(ioport_pin_t pin, ioport_mode_t mode)
{
    ioport_set_pin_mode(pin, mode);
    ioport_disable_pin(pin);
}

static inline void configureGpioPin(ioport_pin_t pin, bool level)
{
    ioport_set_pin_mode(pin, 0);  // GPIO_MODE_OUTPUT_PUSH_PULL
    ioport_set_pin_level(pin, level);
    ioport_set_pin_dir(pin, IOPORT_DIR_OUTPUT);
    ioport_enable_pin(pin);
}

static inline void setLevelShifterWrite(PlatformSpiDefinition_t *self)
{
    // this function assumes that ls1_dir level has been previously set to high
    ioport_set_pin_dir(self->pins.ls1_dir, IOPORT_DIR_OUTPUT);  // logic high
    ioport_set_pin_dir(self->pins.ls2_dir, IOPORT_DIR_INPUT);   // pull-up
}

static inline void setLevelShifterRead(PlatformSpiDefinition_t *self)
{
    // this function assumes that ls2_dir level has been previously set to low
    ioport_set_pin_dir(self->pins.ls1_dir, IOPORT_DIR_INPUT);   // pull-down
    ioport_set_pin_dir(self->pins.ls2_dir, IOPORT_DIR_OUTPUT);  // logic low
}

static inline void setLevelShifterDefault(PlatformSpiDefinition_t *self)
{
    ioport_set_pin_dir(self->pins.ls1_dir, IOPORT_DIR_INPUT);  // pull-up: MISO in read direction
    ioport_set_pin_dir(self->pins.ls2_dir, IOPORT_DIR_INPUT);  // pull-down: MOSI and RST_N in write direction
}

/* \brief pull CS_N low (protected, only to be used in implementing classes) */
static inline void spi_cs_enable(PlatformSpiDefinition_t *self)
{
    ioport_set_pin_level(self->pins.csn, !(m_csnIdleLevel));
}

/* \brief pull CS_N high (protected, only to be used in implementing classes) */
static inline void spi_cs_disable(PlatformSpiDefinition_t *self)
{
    ioport_set_pin_level(self->pins.csn, m_csnIdleLevel);
}

/* \brief acquire lock to spi object to prevent access conflicts
 * (protected, only to be used internally) */
static inline bool spi_try_lock(PlatformSpiDefinition_t *self)
{
    if (self->state.lock == 0)
    {
        self->state.lock++;
        if (self->state.lock == 1)
        {
            return true;
        }
        self->state.lock--;
    }

    return false;
}

static inline void spi_lock(PlatformSpiDefinition_t *self)
{
    while (!spi_try_lock(self))
    {
    }
}

/* \brief release lock to spi object so that other functions can now access it
 * (protected, only to be used internally) */
static inline void spi_unlock(PlatformSpiDefinition_t *self)
{
    self->state.lock--;
}

static enum status_code setup_qspi(PlatformSpiDefinition_t *self, uint8_t flags, uint8_t wordSize, uint32_t baudrate, enum qspi_run_mode mode)
{
    Qspi *qspi = (Qspi *)self->addr.peripheral;

    struct qspi_config_t qspi_config = {
        .serial_memory_mode          = mode,
        .loopback_en                 = false,
        .wait_data_for_transfer      = false,
        .csmode                      = QSPI_LASTXFER,  // best setting even though this implementation handles CS_N as GPIO
        .min_delay_qcs               = MIN_DELAY_QCS,
        .delay_between_ct            = 0,
        .clock_polarity              = (flags & SPI_CPOL),
        .clock_phase                 = (flags & SPI_CPHA),
        .baudrate                    = baudrate,
        .transfer_delay              = TXDELAY,
        .scrambling_en               = false,
        .scrambling_random_value_dis = false,
        .scrambling_user_key         = 0,
        .bits_per_transfer           = (uint32_t)(wordSize - 8) << QSPI_NBITS_CALC_SHIFT,
    };
    return qspi_initialize(qspi, &qspi_config);
}

static void PlatformSpi_setQspiMode(PlatformSpiDefinition_t *self, enum qspi_run_mode mode)
{
    if (getQspiMode(self) == mode)
    {
        return;  // mode is already configured
    }

    if (mode == QSPI_MR_SMM_MEMORY)  // Quad SPI
    {
        configurePeripheralPin(self->pins.dio3, self->pins.dio3_flags);  // also used as RSTN
    }
    else
    {
        configureGpioPin(self->pins.dio3, true);  // prevent accidental reset
    }

    qspi_switch_mode((Qspi *)self->addr.peripheral, mode);
}

static enum status_code setup_spi(PlatformSpiDefinition_t *self, uint8_t flags, uint8_t wordSize, uint32_t baudrate)
{
    int16_t baudrate_div = spi_calc_baudrate_div(baudrate, sysclk_get_peripheral_hz());
    if (baudrate_div < 0)
    {
        return ERR_INVALID_ARG;
    }

    const uint8_t uc_dlybct  = 0;  // Delay between consecutive transfers
    const uint32_t ul_pcs_ch = 0;  // Peripheral Chip Select channel (0~3)

    Spi *spi = (Spi *)self->addr.peripheral;
    spi_enable_clock(spi);
    spi_reset(spi);
    spi_set_master_mode(spi);
    spi_disable_mode_fault_detect(spi);
    spi_disable_loopback(spi);
    //spi_set_peripheral_chip_select_value(spi, spi_get_pcs(ul_pcs_ch));  // setting skipped because this implementation handles CS_N as GPIO
    spi_set_fixed_peripheral_select(spi);
    spi_disable_peripheral_select_decode(spi);
    spi_set_delay_between_chip_select(spi, MIN_DELAY_QCS);
    spi_set_transfer_delay(spi, ul_pcs_ch, TXDELAY, uc_dlybct);
    set_word_size(self, wordSize);
    spi_set_baudrate_div(spi, ul_pcs_ch, (uint8_t)baudrate_div);
    //spi_configure_cs_behavior(spi, ul_pcs_ch, (uint32_t)SPI_CS_KEEP_LOW);  // setting skipped because this implementation handles CS_N as GPIO
    spi_set_clock_polarity(spi, ul_pcs_ch, (flags & SPI_CPOL));
    //lint -e{730} -e{506}
    spi_set_clock_phase(spi, ul_pcs_ch, !(flags & SPI_CPHA));
    spi_enable(spi);

    return STATUS_OK;
}

static void startQspiInstruction(PlatformSpiDefinition_t *self, uint32_t waitCycles, uint8_t code, bool enableCode, bool enableData, bool readInstruction, uint8_t wordSize)
{
    // configure direction of data lines
    if (readInstruction)
    {
        setLevelShifterRead(self);
    }
    else
    {
        setLevelShifterWrite(self);
    }
    spi_cs_enable(self);                                // ensure CS_N is low, to prevent accidental reset when DIO3 goes low
    PlatformSpi_setQspiMode(self, QSPI_MR_SMM_MEMORY);  // reconfigure peripheral as Quad SPI
    set_word_size(self, wordSize);

    Qspi *qspi                             = (Qspi *)self->addr.peripheral;
    struct qspi_mem_cmd_t instruction_code = {
        .instruction = code,
        .option      = 0,
    };
    qspi_set_instruction_code(qspi, instruction_code);

    struct qspi_inst_frame_t qspi_frame = {
        .inst_frame.bm.b_inst_en        = enableCode,  // 8-bit instruction code
        .inst_frame.bm.b_addr_en        = false,       // do not transmit address field
        .inst_frame.bm.b_dummy_cycles   = waitCycles,
        .inst_frame.bm.b_width          = 6,           // QSPI_IFR_WIDTH_QUAD_CMD: Instruction: Quad SPI / Address-Option: Quad SPI / Data: Quad SPI
        .inst_frame.bm.b_data_en        = enableData,  // data will be received from device
        .inst_frame.bm.b_opt_en         = false,       // do not transmit option code
        .inst_frame.bm.b_opt_len        = 3,           // QSPI_IFR_OPTL_OPTION_8BIT: 8-bit option code length (not used but needed for compatibility)
        .inst_frame.bm.b_tfr_type       = 0,           // QSPI_IFR_TFRTYP_TRSFR_READ: read transfer mode 0
        .inst_frame.bm.b_continues_read = false,       // disable continuous read mode
    };
    qspi_set_instruction_frame(qspi, qspi_frame);

    if (enableData)
    {
        qspi_get_inst_frame(qspi);  // synchronize system bus accesses before data readout
    }
}

static sr_t _transfer8(PlatformSpiDefinition_t *self, const uint8_t *txd, uint8_t *rxd, uint32_t count)
{
    // verified each register and bits matched for QSPI and SPI
    Spi *spi = (Spi *)self->addr.peripheral;

    while (count--)
    {
        uint16_t tx;
        if (txd)
        {
            tx = *txd++;
        }
        else
        {
            tx = 0;
        }
        if (spi_write(spi, tx, 1, 0) != SPI_OK)
        {
            return E_TIMEOUT;
        }

        uint8_t p_pcs;
        uint16_t rx;
        if (spi_read(spi, &rx, &p_pcs) != SPI_OK)
        {
            return E_TIMEOUT;
        }
        if (rxd)
        {
            *rxd++ = (uint8_t)(rx);
        }
    }

    return E_SUCCESS;
}

static sr_t _transfer16(PlatformSpiDefinition_t *self, const uint16_t *txd, uint16_t *rxd, uint32_t count)
{
    // verified each register and bits matched for QSPI and SPI
    Spi *spi = (Spi *)self->addr.peripheral;

    while (count--)
    {
        uint16_t tx;
        if (txd)
        {
            tx = *txd++;
        }
        else
        {
            tx = 0;
        }
        if (spi_write(spi, tx, 1, 0) != SPI_OK)
        {
            return E_TIMEOUT;
        }

        uint8_t p_pcs;
        uint16_t rx;
        if (spi_read(spi, &rx, &p_pcs) != SPI_OK)
        {
            return E_TIMEOUT;
        }
        if (rxd)
        {
            *rxd++ = rx;
        }
    }

    return E_SUCCESS;
}

// validated spi and qspi same; all registers matching
static void drop_rx(PlatformSpiDefinition_t *self)
{
    Spi *spi = (Spi *)self->addr.peripheral;
    while ((spi->SPI_SR & SPI_SR_RDRF) != 0)
    {
        uint32_t readDummy = spi->SPI_RDR;
        UNUSED(readDummy);
    }
}

// wordSize has to be within [8..16] (it is checked already by the calling function)
static void set_word_size(PlatformSpiDefinition_t *self, uint8_t wordSize)
{
    if (self->peripheral_id == ID_QSPI)
    {
        Qspi *qspi    = (Qspi *)self->addr.peripheral;
        uint32_t bits = (uint32_t)(wordSize - 8) << QSPI_NBITS_CALC_SHIFT;  // to arrive at define QSPI_MR_NBBITS_<wordSize>_BIT
        uint32_t mask = qspi->QSPI_MR & (~QSPI_MR_NBBITS_Msk);
        qspi->QSPI_MR = mask | bits;
    }
    else  // ID_SPI1
    {
        Spi *spi         = (Spi *)self->addr.peripheral;
        uint32_t ul_bits = (uint32_t)((wordSize - 8) << SPI_NBITS_CALC_SHIFT);  // to arrive at define SPI_CSR_BITS_<wordSize>_BIT
        spi_set_bits_per_transfer(spi, 0, ul_bits);
    }
}

void XDMAC_Handler(void)
{
    if (m_devIdIsr >= m_count)
    {
        return;
    }

    PlatformSpiDefinition_t *device = &m_definition[m_devIdIsr];
    uint32_t dma_status             = xdmac_channel_get_interrupt_status(XDMAC, device->dma.rx_dma_channel);

    // Handle "end of linked list execution" event. This occurs only once when DMA finishes.
    if (dma_status & XDMAC_CIS_LIS)
    {
        if (isQspiConfigured(device))
        {
            __DSB();  // wait for all memory operations (including cache) to complete
            finishQspiReadout(device);
            setLevelShifterDefault(device);                    // restore default direction of RST_N, MOSI and MISO pins
            PlatformSpi_setQspiMode(device, QSPI_MR_SMM_SPI);  // reconfigure as regular SPI
        }

        spi_cs_disable(device);
        spi_unlock(device);

        m_devIdIsr = PLATFORM_SPI_MAX_DEVICES;  // reset

        if (device->callback.fn != NULL)
        {
            device->callback.fn(device->callback.context);
        }
        device->callback.fn = NULL;
    }

    // Handle "end of block execution" event. This occurs for each block within a linked list.
    if (dma_status & XDMAC_CIS_BIS)
    {
        // do nothing
    }

    // Handle errors: read/write or request overflow
    if ((dma_status & (XDMAC_CIS_RBEIS | XDMAC_CIS_WBEIS | XDMAC_CIS_ROIS)))
    {
        // do nothing
    }
}

static void dma_transfer(PlatformSpiDefinition_t *self, uint32_t transfer_length, void *rx_data, uint32_t width)
{
    /* We are only interested in reading data from the sensor.
     * 
     * Because regular SPI transfers are bidirectional, a dummy TX DMA
     * is required by the peripheral. Unlike RX, the TX transfer consists
     * of writing a dummy value into the peripheral register, repeatedly,
     * for the length of underlying DMA transfer.
     */

    //lint -e{550} -e{830}
    static uint32_t dummy_zero = 0;

    /*lint -e{845} */
    const uint32_t mbr_cfg = XDMAC_CC_TYPE_PER_TRAN |
                             XDMAC_CC_MBSIZE_SINGLE |
                             XDMAC_CC_CSIZE_CHK_1 |
                             width;

    /*lint -e{701} -e{845} */
    const uint32_t tx_mbr_cfg = mbr_cfg |
                                XDMAC_CC_DSYNC_MEM2PER |
                                XDMAC_CC_SIF_AHB_IF0 |
                                XDMAC_CC_DIF_AHB_IF1 |
                                XDMAC_CC_SAM_FIXED_AM |
                                XDMAC_CC_DAM_FIXED_AM |
                                XDMAC_CC_PERID(self->dma.tx_dma_hw_id);

    /*lint -e{701} -e{845} */
    const uint32_t rx_mbr_cfg = mbr_cfg |
                                XDMAC_CC_DSYNC_PER2MEM |
                                XDMAC_CC_SIF_AHB_IF1 |
                                XDMAC_CC_DIF_AHB_IF0 |
                                XDMAC_CC_SAM_FIXED_AM |
                                XDMAC_CC_DAM_INCREMENTED_AM |
                                XDMAC_CC_PERID(self->dma.rx_dma_hw_id);

    uint32_t channelMask = 0;

    /* Initialize TX dma */
    self->dma.dma_tx.mbr_nda = 0;  // this terminates the linked list
    self->dma.dma_tx.mbr_ubc = XDMAC_UBC_UBLEN(transfer_length);
    self->dma.dma_tx.mbr_sa  = (uint32_t)&dummy_zero;
    self->dma.dma_tx.mbr_da  = self->addr.tdr;
    /*lint -e{701} -e{845} */
    self->dma.dma_tx.mbr_cfg = tx_mbr_cfg;

    xdmac_channel_config_t xdmac_tx_cfg = {
        .mbr_ubc = 0,
        .mbr_sa  = 0,
        .mbr_da  = self->addr.tdr,
        .mbr_cfg = 0,
        .mbr_bc  = 0,
        .mbr_ds  = 0,
        .mbr_sus = 0,
        .mbr_dus = 0,
    };
    xdmac_configure_transfer(XDMAC, self->dma.tx_dma_channel, &xdmac_tx_cfg);

    xdmac_channel_set_descriptor_control(XDMAC, self->dma.tx_dma_channel,
                                         XDMAC_CNDC_NDVIEW_NDV2 |
                                             XDMAC_CNDC_NDE_DSCR_FETCH_EN |
                                             XDMAC_CNDC_NDSUP_SRC_PARAMS_UPDATED);
    xdmac_channel_set_descriptor_addr(XDMAC, self->dma.tx_dma_channel, (uint32_t)(&self->dma.dma_tx), 0);

    channelMask |= XDMAC_GE_EN0 << self->dma.tx_dma_channel;

    /* Initialize RX dma */
    self->dma.dma_rx.mbr_nda = 0;  // this terminates the linked list
    self->dma.dma_rx.mbr_ubc = XDMAC_UBC_UBLEN(transfer_length);
    self->dma.dma_rx.mbr_da  = (uint32_t)rx_data;
    self->dma.dma_rx.mbr_sa  = 0;
    self->dma.dma_rx.mbr_cfg = rx_mbr_cfg;

    xdmac_channel_config_t xdmac_rx_cfg = {
        xdmac_rx_cfg.mbr_ubc = 0,
        xdmac_rx_cfg.mbr_da  = 0,
        xdmac_rx_cfg.mbr_sa  = self->addr.rdr,
        xdmac_rx_cfg.mbr_cfg = 0,
        xdmac_rx_cfg.mbr_bc  = 0,
        xdmac_rx_cfg.mbr_ds  = 0,
        xdmac_rx_cfg.mbr_sus = 0,
        xdmac_rx_cfg.mbr_dus = 0,
    };
    xdmac_configure_transfer(XDMAC, self->dma.rx_dma_channel, &xdmac_rx_cfg);

    xdmac_channel_set_descriptor_control(XDMAC, self->dma.rx_dma_channel,
                                         XDMAC_CNDC_NDVIEW_NDV2 |
                                             XDMAC_CNDC_NDE_DSCR_FETCH_EN |
                                             XDMAC_CNDC_NDDUP_DST_PARAMS_UPDATED);
    xdmac_channel_set_descriptor_addr(XDMAC, self->dma.rx_dma_channel, (uint32_t)(&self->dma.dma_rx), 0);

    channelMask |= XDMAC_GE_EN0 << self->dma.rx_dma_channel;

#ifdef CONF_BOARD_ENABLE_CACHE_AT_INIT
    /* Update DCache before DMA transmit */
    SCB_CleanInvalidateDCache();
#endif

    /* Start DMA Transfer */
    XDMAC->XDMAC_GE = channelMask;

    //lint -e{550} -e{830}
}

static void dma_transfer8(PlatformSpiDefinition_t *self, uint32_t transfer_length, uint8_t *rx_data)
{
    dma_transfer(self, transfer_length, rx_data, XDMAC_CC_DWIDTH_BYTE);
}

static void dma_transfer16(PlatformSpiDefinition_t *self, uint32_t transfer_length, uint16_t *rx_data)
{
    dma_transfer(self, transfer_length, rx_data, XDMAC_CC_DWIDTH_HALFWORD);
}

static void dma_receive32(PlatformSpiDefinition_t *self, uint32_t transfer_length, uint32_t *rx_data)
{
    /* Initiates a unidirectional memory to memory DMA transfer using QSPIMEM_ADDR as source.
     * In order to achieve best performance and reduce gaps between QSPI clock cycles,
     * the full data bus capacity is used by doing 32-bit word transfers.
     * 
     * Note: this memory-mapped data transfer is only supported if Quad SPI is configured.
     */

    const uint32_t sourceAddress = QSPIMEM_ADDR;
    const uint32_t channel       = self->dma.rx_dma_channel;
    const uint32_t channel_cfg   = XDMAC_CC_TYPE_MEM_TRAN |     // memory-to-memory transfer
                                 XDMAC_CC_SIF_AHB_IF1 |         // bus interface to source (QSPI)
                                 XDMAC_CC_DIF_AHB_IF0 |         // bus interface to destination (memory)
                                 XDMAC_CC_SAM_INCREMENTED_AM |  // memory addressing mode to source
                                 XDMAC_CC_DAM_INCREMENTED_AM |  // memory addressing mode to destination
                                 XDMAC_CC_DWIDTH_WORD |         // transfer data width 32 bits
                                 XDMAC_CC_MBSIZE_SIXTEEN;       // group up to 16 words in a memory burst when possible

    xdmac_channel_config_t transfer_cfg = {
        .mbr_ubc = XDMAC_UBC_UBLEN(transfer_length),  // number of words transferred by the microblock
        .mbr_da  = (uint32_t)rx_data,
        .mbr_sa  = sourceAddress,
        .mbr_cfg = channel_cfg,
        .mbr_bc  = 0,  // single microblock transfer
        .mbr_ds  = 0,  // no striding
        .mbr_sus = 0,  // no striding
        .mbr_dus = 0,  // no striding
    };
    xdmac_configure_transfer(XDMAC, channel, &transfer_cfg);
    xdmac_channel_set_descriptor_control(XDMAC, channel, 0);  // single block transfer i.e. no linked list

    const uint32_t channelMask = XDMAC_GE_EN0 << channel;

#ifdef CONF_BOARD_ENABLE_CACHE_AT_INIT
    /* Update DCache before DMA transmit */
    SCB_CleanInvalidateDCache();
#endif

    /* Start DMA Transfer */
    XDMAC->XDMAC_GE = channelMask;
}

/******************************************************************************/
/*Interface Methods Definition -----------------------------------------------*/
/******************************************************************************/

sr_t PlatformSpi_readBurstAsync8(uint8_t devId, uint8_t *bufRead, uint32_t count, void (*cb)(void *), void *arg)
{
    if (devId >= m_count)
    {
        return E_OUT_OF_BOUNDS;
    }
    if (!m_deviceConfigured[devId])
    {
        return E_NOT_CONFIGURED;
    }

    if (count == 0)
    {
        return E_SUCCESS;
    }

    if (m_devIdIsr < PLATFORM_SPI_MAX_DEVICES)
    {
        return E_BUSY;
    }
    m_devIdIsr = devId;

    PlatformSpiDefinition_t *device = &m_definition[devId];

    /* SPI lock is released in DMA IRQ, so no spi_unlock(device) call in this routine */
    if (!spi_try_lock(device))
    {
        m_devIdIsr = PLATFORM_SPI_MAX_DEVICES;
        return E_BUSY;
    }

    device->callback.context = arg;
    device->callback.fn      = cb;

    if (isQspiConfigured(device))
    {
        /* Quad SPI best performance is achieved doing 32-bit transfers.
         * This requires a 32-bit aligned buffer and a byte count multiple of 4.
         */
        void *buffer = bufRead;
        if ((buffer != (uint32_t *)buffer) || (count % sizeof(uint32_t)))
        {
            return E_NOT_POSSIBLE;
        }

        const bool readInstruction = true;
        const bool enableData      = true;
        const bool enableCode      = false;
        const uint32_t waitCycles  = m_qspiReadoutWaitCycles;
        const uint8_t unusedCode   = 0;
        const uint8_t wordSize     = 8;
        const uint32_t wordCount   = count / sizeof(uint32_t);
        startQspiInstruction(device, waitCycles, unusedCode, enableCode, enableData, readInstruction, wordSize);
        dma_receive32(device, wordCount, (uint32_t *)(uintptr_t)bufRead);
    }
    else
    {
        /* drop any leftover content of the SPI RX registers to make sure we don't get any old data in the next transfer */
        drop_rx(device);
        dma_transfer8(device, count, bufRead);
    }

    return E_SUCCESS;
}

sr_t PlatformSpi_readBurstAsync12(uint8_t devId, uint16_t *bufRead, uint32_t count, void (*cb)(void *), void *arg)
{
    if (devId >= m_count)
    {
        return E_OUT_OF_BOUNDS;
    }
    if (!m_deviceConfigured[devId])
    {
        return E_NOT_CONFIGURED;
    }

    if (count == 0)
    {
        return E_SUCCESS;
    }

    if (m_devIdIsr < PLATFORM_SPI_MAX_DEVICES)
    {
        return E_BUSY;
    }
    m_devIdIsr = devId;

    PlatformSpiDefinition_t *device = &m_definition[devId];

    /* SPI lock is released in DMA IRQ, so no spi_unlock(device) call in this routine */
    if (!spi_try_lock(device))
    {
        m_devIdIsr = PLATFORM_SPI_MAX_DEVICES;
        return E_BUSY;
    }

    device->callback.context = arg;
    device->callback.fn      = cb;

    const uint8_t wordSize = 12;
    if (isQspiConfigured(device))
    {
        // the Atmel MCU does not support wordSize = 12 according to documentation
        spi_unlock(device);
        return E_NOT_IMPLEMENTED;
    }
    else
    {
        /* drop any leftover content of the SPI RX registers to make sure we don't get any old data in the next transfer */
        drop_rx(device);
        set_word_size(device, wordSize);
    }

    dma_transfer16(device, count, bufRead);
    return E_SUCCESS;
}

sr_t PlatformSpi_readBurstAsync16(uint8_t devId, uint16_t *bufRead, uint32_t count, void (*cb)(void *), void *arg)
{
    if (devId >= m_count)
    {
        return E_OUT_OF_BOUNDS;
    }
    if (!m_deviceConfigured[devId])
    {
        return E_NOT_CONFIGURED;
    }

    if (count == 0)
    {
        return E_SUCCESS;
    }

    if (m_devIdIsr < PLATFORM_SPI_MAX_DEVICES)
    {
        return E_BUSY;
    }
    m_devIdIsr = devId;

    PlatformSpiDefinition_t *device = &m_definition[devId];

    /* SPI lock is released in DMA IRQ, so no spi_unlock(device) call in this routine */
    if (!spi_try_lock(device))
    {
        m_devIdIsr = PLATFORM_SPI_MAX_DEVICES;
        return E_BUSY;
    }

    device->callback.context = arg;
    device->callback.fn      = cb;

    const uint8_t wordSize = 16;
    if (isQspiConfigured(device))
    {
        /* Quad SPI best performance is achieved doing 32-bit transfers.
         * This requires a 32-bit aligned buffer and a byte count multiple of 4.
         */
        void *buffer = bufRead;
        if ((buffer != (uint32_t *)buffer) || (count % sizeof(uint32_t)))
        {
            return E_NOT_POSSIBLE;
        }

        const bool readInstruction = true;
        const bool enableData      = true;
        const bool enableCode      = false;
        const uint32_t waitCycles  = m_qspiReadoutWaitCycles;
        const uint8_t unusedCode   = 0;
        const uint32_t wordCount   = count / sizeof(uint32_t);
        startQspiInstruction(device, waitCycles, unusedCode, enableCode, enableData, readInstruction, wordSize);
        dma_receive32(device, wordCount, (uint32_t *)(uintptr_t)bufRead);
    }
    else
    {
        /* drop any leftover content of the SPI RX registers to make sure we don't get any old data in the next transfer */
        drop_rx(device);
        set_word_size(device, wordSize);
    }

    dma_transfer16(device, count, bufRead);
    return E_SUCCESS;
}

uint32_t PlatformSpi_getMaxTransfer(void)
{
    return PlatformSpi_MAX_TRANSFER;
}

sr_t PlatformSpi_configure(uint8_t devId, uint8_t flags, uint8_t wordSize, uint32_t speed)
{
    if (devId >= m_count)
    {
        return E_OUT_OF_BOUNDS;
    }

    if ((wordSize < 8) || (wordSize > 16))
    {
        return E_INVALID_PARAMETER;
    }

    PlatformSpiDefinition_t *device = &m_definition[devId];
    if (speed > device->baudrate)
    {
        // silently clip the device speed to the maximum allowed
        speed = device->baudrate;
    }

    enum status_code status;
    spi_lock(device);
    switch (device->peripheral_id)
    {
        case ID_QSPI:
            status = setup_qspi(device, flags, wordSize, speed, spi_mode);  // default (Quad SPI mode is disabled))
            break;
        case ID_SPI1:
            status = setup_spi(device, flags, wordSize, speed);
            break;
        case ID_SPI0:
            status = setup_spi(device, flags, wordSize, speed);
            break;
        default:
            status = ERR_INVALID_ARG;
            break;
    }
    spi_unlock(device);

    if (status != STATUS_OK)
    {
        return E_INVALID_PARAMETER;
    }

    m_deviceConfigured[devId] = true;
    return E_SUCCESS;
}

sr_t PlatformSpi_write8(uint8_t devId, uint32_t count, const uint8_t buffer[], bool keepSel)
{
    //Input buffer is NULL -> do not care about incoming data
    return PlatformSpi_transfer8(devId, count, buffer, NULL, keepSel);
}

sr_t PlatformSpi_write16(uint8_t devId, uint32_t count, const uint16_t buffer[], bool keepSel)
{
    //Input buffer is NULL -> do not care about incoming data
    return PlatformSpi_transfer16(devId, count, buffer, NULL, keepSel);
}

sr_t PlatformSpi_write32(uint8_t devId, uint32_t count, const uint32_t buffer[], bool keepSel)
{
    //Input buffer is NULL -> do not care about incoming data
    return PlatformSpi_transfer32(devId, count, buffer, NULL, keepSel);
}

sr_t PlatformSpi_read8(uint8_t devId, uint32_t count, uint8_t buffer[], bool keepSel)
{
    //Output buffer is NULL -> send all zeroes at every transfer
    return PlatformSpi_transfer8(devId, count, NULL, buffer, keepSel);
}

sr_t PlatformSpi_read16(uint8_t devId, uint32_t count, uint16_t buffer[], bool keepSel)
{
    //Output buffer is NULL -> send all zeroes at every transfer
    return PlatformSpi_transfer16(devId, count, NULL, buffer, keepSel);
}

sr_t PlatformSpi_read32(uint8_t devId, uint32_t count, uint32_t buffer[], bool keepSel)
{
    //Output buffer is NULL -> send all zeroes at every transfer
    return PlatformSpi_transfer32(devId, count, NULL, buffer, keepSel);
}

sr_t PlatformSpi_writeQspi8(uint8_t devId, uint32_t count, const uint8_t buffer[], bool keepSel)
{
    if (devId >= m_count)
    {
        return E_OUT_OF_BOUNDS;
    }
    if (!m_deviceConfigured[devId])
    {
        return E_NOT_CONFIGURED;
    }

    if (count != 1)
    {
        return E_NOT_IMPLEMENTED;
    }

    PlatformSpiDefinition_t *self = &m_definition[devId];
    if (self->peripheral_id != ID_QSPI)
    {
        return E_NOT_SUPPORTED;
    }
    Qspi *qspi = (Qspi *)self->addr.peripheral;
    spi_lock(self);

    const bool readInstruction = false;
    const bool enableData      = false;
    const bool enableCode      = true;
    const uint32_t waitCycles  = 0;  // no dummy cycles are needed before writing
    const uint8_t code         = buffer[0];
    const uint8_t wordSize     = 8;
    startQspiInstruction(self, waitCycles, code, enableCode, enableData, readInstruction, wordSize);
    waitQspiInstructionFinish(qspi);

    if (!keepSel)
    {
        setLevelShifterDefault(self);                    // restore default direction of RST_N, MOSI and MISO pins
        PlatformSpi_setQspiMode(self, QSPI_MR_SMM_SPI);  // reconfigure as regular SPI
        spi_cs_disable(self);
    }
    spi_unlock(self);

    return E_SUCCESS;
}

sr_t PlatformSpi_readQspi8(uint8_t devId, uint32_t count, uint8_t buffer[], bool keepSel)
{
    if (devId >= m_count)
    {
        return E_OUT_OF_BOUNDS;
    }
    if (!m_deviceConfigured[devId])
    {
        return E_NOT_CONFIGURED;
    }

    if (count == 0)
    {
        return E_SUCCESS;
    }

    PlatformSpiDefinition_t *self = &m_definition[devId];
    if (self->peripheral_id != ID_QSPI)
    {
        return E_NOT_SUPPORTED;
    }
    spi_lock(self);

    const bool readInstruction = true;
    const bool enableData      = true;
    const bool enableCode      = false;
    const uint32_t waitCycles  = m_qspiReadoutWaitCycles;
    const uint8_t unusedCode   = 0;
    const uint8_t wordSize     = 8;
    startQspiInstruction(self, waitCycles, unusedCode, enableCode, enableData, readInstruction, wordSize);

    uint8_t *qspiData = (uint8_t *)QSPIMEM_ADDR;
    while (count--)
    {
        *buffer++ = *qspiData++;
    }
    finishQspiReadout(self);

    if (!keepSel)
    {
        setLevelShifterDefault(self);                    // restore default direction of RST_N, MOSI and MISO pins
        PlatformSpi_setQspiMode(self, QSPI_MR_SMM_SPI);  // reconfigure as regular SPI
        spi_cs_disable(self);
    }
    spi_unlock(self);

    return E_SUCCESS;
}

sr_t PlatformSpi_transfer8(uint8_t devId, uint32_t count, const uint8_t bufOut[], uint8_t bufIn[], bool keepSel)
{
    if (devId >= m_count)
    {
        return E_OUT_OF_BOUNDS;
    }
    if (!m_deviceConfigured[devId])
    {
        return E_NOT_CONFIGURED;
    }

    if (count == 0)
    {
        return E_SUCCESS;
    }

    PlatformSpiDefinition_t *device = &m_definition[devId];
    spi_lock(device);

    set_word_size(device, 8);
    spi_cs_enable(device);

    const sr_t ret = _transfer8(device, bufOut, bufIn, count);

    if (!keepSel)
    {
        spi_cs_disable(device);
    }
    spi_unlock(device);

    return ret;
}

sr_t PlatformSpi_transfer16(uint8_t devId, uint32_t count, const uint16_t bufOut[], uint16_t bufIn[], bool keepSel)
{
    if (devId >= m_count)
    {
        return E_OUT_OF_BOUNDS;
    }
    if (!m_deviceConfigured[devId])
    {
        return E_NOT_CONFIGURED;
    }

    if (count == 0)
    {
        return E_SUCCESS;
    }

    PlatformSpiDefinition_t *device = &m_definition[devId];
    spi_lock(device);

    set_word_size(device, 16);
    spi_cs_enable(device);

    const sr_t ret = _transfer16(device, bufOut, bufIn, count);

    if (!keepSel)
    {
        spi_cs_disable(device);
    }
    spi_unlock(device);

    return ret;
}

sr_t PlatformSpi_transfer32(uint8_t devId, uint32_t count, const uint32_t bufOut[], uint32_t bufIn[], bool keepSel)
{
    return E_NOT_IMPLEMENTED;
}

sr_t PlatformSpi_initialize(PlatformSpiDefinition_t *definition, uint8_t count)
{
    if (count > PLATFORM_SPI_MAX_DEVICES)
    {
        return E_OUT_OF_BOUNDS;
    }

    m_definition = definition;
    m_count      = count;

    for (unsigned int i = 0; i < m_count; i++)
    {
        PlatformSpiDefinition_t *device = &m_definition[i];

        pmc_enable_periph_clk(device->peripheral_id);

        // chip select is handled as GPIO
        configureGpioPin(device->pins.csn, m_csnIdleLevel);

        // remaining pins are controlled by the peripheral
        configurePeripheralPin(device->pins.clk, device->pins.clk_flags);
        configurePeripheralPin(device->pins.mosi, device->pins.mosi_flags);
        configurePeripheralPin(device->pins.miso, device->pins.miso_flags);
        if (device->pins.dio2 != SPI_PIN_NONE)
        {
            configurePeripheralPin(device->pins.dio2, device->pins.dio2_flags);

            /* The QSPI implementation relies on level shifters to change the signal direction
             * during transactions by calling setLevelShifterWrite() and setLevelShifterRead().
             * These functions assume the corresponding pin levels to be in a predefined state,
             * which is configured here.
             */
            ioport_set_pin_level(device->pins.ls1_dir, true);
            ioport_set_pin_level(device->pins.ls2_dir, false);
        }

        const uint32_t irqMask = XDMAC_CIE_LIE;  // enable DMA linked list IRQ event
        xdmac_enable_interrupt(XDMAC, device->dma.rx_dma_channel);
        xdmac_channel_enable_interrupt(XDMAC, device->dma.rx_dma_channel, irqMask);
    }

    if (m_count > 0)
    {
        // Initialize and enable DMA controller
        pmc_enable_periph_clk(ID_XDMAC);

        // Enable DMA interrupt
        NVIC_ClearPendingIRQ(XDMAC_IRQn);
        NVIC_SetPriority(XDMAC_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(), ISR_PRIORITY_SPI_IRQ, 0));  // Set interrupt Priority
        NVIC_EnableIRQ(XDMAC_IRQn);
    }

    return E_SUCCESS;
}
