#include "DataSmartar.h"

#include <common/typeutils.h>
#include <fatal_error.h>
#include <impl/PlatformInterrupt.h>
#include <impl/PlatformSpi.h>
#include <impl/chrono.h>

#include <stdbool.h>
#include <stddef.h>
#include <string.h>


IData DataSmartar = {
    .configure        = DataSmartar_configure,
    .start            = DataSmartar_start,
    .stop             = DataSmartar_stop,
    .getStatusFlags   = DataSmartar_getStatusFlags,
    .registerCallback = DataSmartar_registerCallback,
};

static IData_callback m_callback                  = NULL;
static void *m_arg                                = NULL;
static IData_acquisitionStatusCallback m_statusCb = NULL;

#define DATA_QUEUE_ELEMENT_TYPE uint16_t
#include <platform/data_queue.h>

#define DATA_SMARTAR_MAX_READOUTS (1)  // number of data readouts that can be performed for every IRQ
#define DATA_SMARTAR_MAX_COUNT    (4)  // number of data interfaces supported (4 flexible pins)
#define FIFO_START_ADDRESS        (0x4000)
#define FIFO_END_ADRESS           (0x7FFF)

#define IR_STAT_ADDRESS             0x0300
#define IR_STAT_FIFO_UNDERFLOW_MASK 0x8000
#define IR_STAT_FIFO_OVERFLOW_MASK  0x10000
#define IR_CLEAR                    0x0304
#define IR_FIFO_CLEAR_BITS          (1u << 7)

typedef struct
{
    DataQueue_t queue;
    volatile uint64_t timestamp;  // timestamp identifying when the data generation occurred
    volatile uint16_t pending;    // flag indicating that acquisition is pending
    volatile sr_t readoutError;   // readout error value (When no readout error occurred equals to E_SUCCESS, otherwise holds the error code).
    volatile bool running;        // flag indicating if acquisition is running
    uint8_t devId;
    const PlatformInterruptDefinition_t *irq;
    uint16_t readouts[DATA_SMARTAR_MAX_READOUTS][2];  // data acquisition readouts to be performed for every IRQ to obtain one slice
    uint16_t readoutEntries;                          //
    uint32_t sliceSize;                               // number of words per frame slice
    uint16_t aggregation;                             // frames may consist of 1 slice or be aggregated from multiple slices
} DataSmartar_t;
static DataSmartar_t m_dataSmartarArray[DATA_SMARTAR_MAX_COUNT] = {{{0}}};

static inline sr_t _read(uint8_t devId, uint16_t address, uint32_t *value)
{
    const uint16_t readCommand[2] = SMARTAR_READ(address);
    RETURN_ON_ERROR(PlatformSpi_write16(devId, ARRAY_SIZE(readCommand), readCommand, true));
    uint16_t *buffer = (uint16_t *)value;
    RETURN_ON_ERROR(PlatformSpi_read16(devId, sizeof(*value) / sizeof(*buffer), buffer, false));
    *value = (*value << 16) | (*value >> 16);

    return E_SUCCESS;
}

static inline sr_t _write(uint8_t devId, uint16_t address, uint32_t value)
{
    const uint16_t writeCommand[2] = SMARTAR_WRITE(address);
    const uint16_t writeValue[2]   = SMARTAR_WRITE_VALUE(value);
    RETURN_ON_ERROR(PlatformSpi_write16(devId, ARRAY_SIZE(writeCommand), writeCommand, true));
    RETURN_ON_ERROR(PlatformSpi_write16(devId, sizeof(value) / sizeof(*writeValue), writeValue, false));

    return E_SUCCESS;
}

/** @brief Returns E_SUCCESS if no FIFO overflow or underflow occurred
 *
 * @retval      E_OVERFLOW      If a FIFO overflow or underflow occurred
 * @retval      E_SUCCESS       If no FIFO overflow or underflow occured occurred
 */
static inline sr_t getFifoError(uint8_t devId)
{
    uint32_t interruptStatus;
    RETURN_ON_ERROR(_read(devId, IR_STAT_ADDRESS, &interruptStatus));
    /* Shows if FIFO overflow or underflow condition occurred.
     */
    if (interruptStatus & IR_STAT_FIFO_OVERFLOW_MASK)
    {
        return E_OVERFLOW;
    }
    if (interruptStatus & IR_STAT_FIFO_UNDERFLOW_MASK)
    {
        return E_UNDERFLOW;
    }

    return E_SUCCESS;
}

static inline sr_t clearFifoInterrupt(uint8_t devId)
{
    uint32_t irClearValue;
    RETURN_ON_ERROR(_read(devId, IR_CLEAR, &irClearValue));
    return _write(devId, IR_CLEAR, irClearValue | IR_FIFO_CLEAR_BITS);
}

static inline void acquisitionStatus(bool state)
{
    if (m_statusCb)
    {
        m_statusCb(state);
    }
}

static inline void errorCallback(uint32_t code, uint8_t channel, uint64_t timestamp)
{
    m_callback(m_arg, NULL, code, channel, timestamp);
}

static inline void frameCallback(uint16_t *data, uint32_t size, uint8_t channel, uint64_t timestamp)
{
    uint8_t *payload      = (uint8_t *)(uintptr_t)data;
    const uint32_t length = size * sizeof(*data);
    m_callback(m_arg, payload, length, channel, timestamp);
}

static void DataSmartar_dataReadCallback(void *arg)
{
    DataSmartar_t *self = arg;
    DataQueue_t *queue  = &self->queue;

    // Check that no underflow or overflow occurred
    self->readoutError = getFifoError(self->devId);
    if (self->readoutError == E_SUCCESS)
    {
        /* update write index for the next data fetch */
        queue_updateWritePointer(queue, self->sliceSize);

        // manually clear the FIFO interrupt
        self->readoutError = clearFifoInterrupt(self->devId);
    }

    acquisitionStatus(false);
}

static void DataSmartar_readData(DataSmartar_t *self, uint16_t *data)
{
    /* Do nothing if data acquisition is not running.
     */
    if (!self->running)
    {
        return;
    }

    /* It cannot happen here that the queue is full. If the queue is full
     * this function will not get invoked until the queue is no longer full.
     */

    acquisitionStatus(true);

    /* Initiates an SPI burst read by sending the read command. */
    const uint16_t command[2] = SMARTAR_READ(self->readouts[0][0]);
    const uint16_t count      = self->readouts[0][1];

    self->readoutError = PlatformSpi_write16(self->devId, ARRAY_SIZE(command), command, true);
    if (self->readoutError == E_SUCCESS)
    {
        self->readoutError = PlatformSpi_readBurstAsync16(self->devId, data, count, DataSmartar_dataReadCallback, self);
    }

    if (self->readoutError != E_SUCCESS)
    {
        // an error occurred, so we do not continue and let the run function handle it
        return;
    }
}

static void DataSmartar_newDataCallback(void *arg)
{
    /* Data will be fetched by the main loop function DataSmartar_checkReadData() */
    DataSmartar_t *self = (DataSmartar_t *)arg;

    /* The timestamp is saved if it was not previously set. */
    if (!self->timestamp)
    {
        self->timestamp = chrono_ticks_to_microseconds(chrono_now());
    }

    if (self->pending)
    {
        /* A previous fetch request is already pending or currently being serviced
         * in the main loop (function DataSmartar_run). Therefore the pending counter
         * shall be simply incremented, to avoid a race condition.
         */
        self->pending++;
        return;
    }

    DataQueue_t *queue            = &self->queue;
    const uint32_t writeAvailable = queue_writeAvailable(queue);
    const uint16_t readoutSize    = self->sliceSize;

    if (writeAvailable >= readoutSize)
    {
        uint16_t *data = queue_getWritePointer(queue);
        DataSmartar_readData(self, data);
    }
    else
    {
        /* Queue is full, no space left for another package, so do *not* fetch data yet.
         * This means the BGT FIFO won't be emptied until a package is read from the queue
         * in the main loop (function DataSmartar_run). If this takes too long the BGT FIFO
         * will have an overflow.
         */
        self->pending++;
    }
}

void DataSmartar_run(void)
{
    if (!m_callback)
    {
        return;
    }

    for (unsigned int index = 0; index < DATA_SMARTAR_MAX_COUNT; index++)
    {
        DataSmartar_t *self = &m_dataSmartarArray[index];

        if (!self->running)
        {
            continue;
        }

        if (self->readoutError != E_SUCCESS)
        {
            DataSmartar_stop(index);
            errorCallback(self->readoutError, index, self->timestamp);
            continue;
        }

        DataQueue_t *queue        = &self->queue;
        const uint32_t queueCount = queue_readAvailable(queue);
        const uint32_t frameSize  = self->sliceSize * self->aggregation;
        if (queueCount < frameSize)
        {
            // complete frame is not yet available
            continue;
        }

        uint16_t *data           = queue_getReadPointer(queue);
        const uint64_t timestamp = self->timestamp;
        /* Reset timestamp to indicate that timestamp for the next frame slice needs to be acquired.
         * This should be done as soon as possible, since an interrupt can occur during the callback.
         */
        self->timestamp = 0;
        frameCallback(data, frameSize, index, timestamp);

        /* Update the queue's read position, which also frees the memory of the consumed data */
        queue_updateReadPointer(queue, frameSize);

        /* If the queue was full, a data fetch request might be pending,
         * hence we invoke it here now that queue space has been freed.
         */
        if (self->pending)
        {
            uint16_t *ptr = queue_getWritePointer(queue);
            DataSmartar_readData(self, ptr);
            self->pending--;
        }
    }
}

sr_t DataSmartar_configure(uint8_t index, const IDataProperties_t *dataProperties, const uint8_t *settings, uint16_t settingsSize)
{
    /* Configures the readout parameters for data to be read from the device.
     */

    if (index >= DATA_SMARTAR_MAX_COUNT)
    {
        return E_OUT_OF_BOUNDS;
    }

    DataSmartar_stop(index);

    DataSmartar_t *self  = &m_dataSmartarArray[index];
    self->readoutEntries = 0;  // disable configuration

    const uint16_t readoutEntrySize = sizeof(*self->readouts);
    if ((settingsSize % readoutEntrySize) || (settings == NULL))
    {
        return E_INVALID_SIZE;
    }

    /* The last entry might contain an optional aggregation setting.
     * This is indicated by field [1] (readout count) equal to zero.
     * In such case, field [0] contains the setting.
     */
    uint16_t readoutEntries   = settingsSize / readoutEntrySize;
    const uint16_t *lastEntry = (const uint16_t *)(uintptr_t)&settings[settingsSize - readoutEntrySize];
    if (lastEntry[1] == 0)
    {
        self->aggregation = lastEntry[0] + 1;
        readoutEntries--;
    }
    else
    {
        self->aggregation = 1;  // no aggregation (default)
    }

    // a local copy of the readout entries is needed by the ISR
    if (readoutEntries > DATA_SMARTAR_MAX_READOUTS)
    {
        return E_INVALID_SIZE;
    }
    memcpy(self->readouts, settings, (readoutEntries * readoutEntrySize));

    // parse and validate readout entries
    const uint16_t i = 0;  // TODO: implement support for multiple entries
    //for (uint16_t i = 0; i < readoutEntries; i++)
    uint16_t readoutCount = self->readouts[i][1];
    if (readoutCount == 0)
    {
        return E_INVALID_PARAMETER;
    }
    self->sliceSize = readoutCount;

    // Make sure not to exceed the SRAM FIFO capactiy
    if (self->sliceSize * self->aggregation > (FIFO_END_ADRESS - FIFO_START_ADDRESS))
    {
        return E_INVALID_PARAMETER;
    }

    // Configure the queue size to ensure that the frame storage is sequential/contiguous.
    DataQueue_t *queue = &self->queue;
    if (!queue_configure(queue, self->sliceSize * self->aggregation))
    {
        // return overflow error if the queue can not store the configured contiguous size
        return E_OVERFLOW;
    }

    self->readoutEntries = readoutEntries;  // enable configuration
    return E_SUCCESS;
}

sr_t DataSmartar_start(uint8_t index)
{
    if (index >= DATA_SMARTAR_MAX_COUNT)
    {
        return E_OUT_OF_BOUNDS;
    }

    DataSmartar_t *self = &m_dataSmartarArray[index];
    if (self->readoutEntries == 0)
    {
        return E_NOT_CONFIGURED;
    }

    queue_reset(&self->queue);

    self->pending      = 0;
    self->readoutError = E_SUCCESS;
    // Initialize the timestamp to 0, so it will be set when a data interrupt occurs
    self->timestamp = 0;
    self->running   = true;
    PlatformInterrupt_enable(self->irq, true);  // enable IRQ

    return E_SUCCESS;
}

sr_t DataSmartar_stop(uint8_t index)
{
    if (index >= DATA_SMARTAR_MAX_COUNT)
    {
        return E_OUT_OF_BOUNDS;
    }

    DataSmartar_t *self = &m_dataSmartarArray[index];
    PlatformInterrupt_enable(self->irq, false);  // disable IRQ
    self->running = false;
    acquisitionStatus(false);

    return E_SUCCESS;
}

sr_t DataSmartar_getStatusFlags(uint8_t index, uint32_t *flags)
{
    // TODO: implement if applicable.
    *flags = 0;
    return E_SUCCESS;
}

sr_t DataSmartar_registerCallback(IData_callback callback, void *arg)
{
    m_callback = callback;
    m_arg      = arg;

    return E_SUCCESS;
}

void DataSmartar_setBuffer(uint8_t index, uint16_t *buffer, uint32_t bufferSize)
{
    if (index >= DATA_SMARTAR_MAX_COUNT)
    {
        fatal_error(FATAL_ERROR_DATA_CONFIG_FAILED);
    }

    DataSmartar_t *self = &m_dataSmartarArray[index];

    queue_initialize(&self->queue, buffer, bufferSize);

    // reset configuration due to changed buffer
    self->readoutEntries = 0;
}

void DataSmartar_initialize(uint8_t index, uint8_t devId, const PlatformInterruptDefinition_t *irq)
{
    if (index >= DATA_SMARTAR_MAX_COUNT)
    {
        fatal_error(FATAL_ERROR_DATA_CONFIG_FAILED);
    }

    DataSmartar_t *self = &m_dataSmartarArray[index];

    self->devId = devId;
    self->irq   = irq;

    sr_t ret = PlatformInterrupt_registerCallback(irq, DataSmartar_newDataCallback, self);
    if (ret != E_SUCCESS)
    {
        fatal_error(FATAL_ERROR_DATA_CONFIG_FAILED);
    }
}

void DataSmartar_Constructor(IData_acquisitionStatusCallback statusCb)
{
    m_statusCb = statusCb;
}
