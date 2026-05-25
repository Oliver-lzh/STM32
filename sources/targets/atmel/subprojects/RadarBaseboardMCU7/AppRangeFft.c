
#include "AppRangeFft.h"
#include "BoardOutput.h"

#include <arm_math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define APP_RANGE_FFT_FFT_SIZE              (64u)
#define APP_RANGE_FFT_SAMPLES_PER_CHIRP     APP_RANGE_FFT_ADC_SAMPLES_PER_CHIRP
#define APP_RANGE_FFT_RX_CHANNELS           APP_RANGE_FFT_ADC_RX_CHANNELS
#define APP_RANGE_FFT_CHIRPS_PER_FRAME      APP_RANGE_FFT_ADC_CHIRPS_PER_FRAME
#define APP_RANGE_FFT_RX_TO_PROCESS         (1u)
#define APP_RANGE_FFT_CHIRP_SAMPLE_COUNT    (APP_RANGE_FFT_SAMPLES_PER_CHIRP * APP_RANGE_FFT_RX_CHANNELS)
#define APP_RANGE_FFT_FRAME_SAMPLE_COUNT    (APP_RANGE_FFT_CHIRPS_PER_FRAME * APP_RANGE_FFT_CHIRP_SAMPLE_COUNT)
#define APP_RANGE_FFT_FRAME_BYTE_COUNT      ((APP_RANGE_FFT_FRAME_SAMPLE_COUNT * 3u) / 2u)
#define APP_RANGE_FFT_CHIRP_BYTE_COUNT      ((APP_RANGE_FFT_CHIRP_SAMPLE_COUNT * 3u) / 2u)
#define APP_RANGE_FFT_DEBUG_RATE_DIVISOR    (16u)
#define APP_RANGE_FFT_RAW_RATE_DIVISOR      (64u)
#define APP_RANGE_FFT_INIT_DELAY_FRAMES     (8u)
#define APP_RANGE_FFT_MATRIX_BIN_COUNT      (APP_RANGE_FFT_FFT_SIZE / 2u)
#define APP_RANGE_FFT_VOTE_CHIRP_COUNT      (10u)
#define APP_RANGE_FFT_SEARCH_START_BIN      (6u)
#define APP_RANGE_FFT_SEARCH_END_BIN        ((APP_RANGE_FFT_FFT_SIZE / 2u) - 1u)
#define APP_RANGE_FFT_TOP_BIN_COUNT         (3u)
#define APP_RANGE_FFT_CLIP_LOW_THRESHOLD    (16u)
#define APP_RANGE_FFT_CLIP_HIGH_THRESHOLD   (4079u)
#define APP_RANGE_FFT_PRINT_RAW_CHIRP_ONLY  (0u)
#define APP_RANGE_FFT_RAW_PRINT_CHUNK_SIZE  (8u)

static const q15_t g_blackmanQ15[APP_RANGE_FFT_FFT_SIZE] = {
      0,    29,   119,   272,   495,   796,  1183,  1667,
   2257,  2964,  3795,  4758,  5855,  7088,  8454,  9946,
  11552, 13259, 15046, 16890, 18766, 20643, 22491, 24277,
  25968, 27532, 28938, 30158, 31166, 31941, 32468, 32734,
  32734, 32468, 31941, 31166, 30158, 28938, 27532, 25968,
  24277, 22491, 20643, 18766, 16890, 15046, 13259, 11552,
   9946,  8454,  7088,  5855,  4758,  3795,  2964,  2257,
  1667,  1183,   796,   495,   272,   119,    29,     0
};

typedef struct
{
    q15_t real;
    q15_t imag;
} AppRangeFft_ComplexQ15_t;

static uint32_t m_frameCounter = 0u;
static bool m_initialized = false;
static bool m_initFailed = false;
static bool m_pendingFrame = false;
static uint8_t m_pendingFramePacked12[APP_RANGE_FFT_FRAME_BYTE_COUNT];
static uint8_t m_pendingChannel = 0u;
static uint64_t m_pendingTimestamp = 0u;
static AppRangeFft_AdcCube_t m_adcCube;

static arm_cfft_radix2_instance_q15 m_fftInstance;
static q15_t m_fftBuffer[APP_RANGE_FFT_FFT_SIZE * 2u];
static AppRangeFft_ComplexQ15_t m_rangeFftMatrix[APP_RANGE_FFT_CHIRPS_PER_FRAME][APP_RANGE_FFT_MATRIX_BIN_COUNT];
static AppRangeFft_Result_t m_latestResult;

static uint16_t unpack_fifo_word_high12(const uint8_t *word)
{
                        //左移4位获取高8位         //右移4位获取低4位
    return (uint16_t)(((uint16_t)word[0] << 4) | ((uint16_t)word[1] >> 4));
}

static uint16_t unpack_fifo_word_low12(const uint8_t *word)
{                           //保留word1的低四位  拼接word2
    return (uint16_t)((((uint16_t)word[1] & 0x0Fu) << 8) | (uint16_t)word[2]);
}

static int32_t abs_i32(int32_t value)
{
    if (value < 0)
    {
        return -value;
    }

    return value;
}

static q15_t saturate_q15(int32_t value)
{
    if (value > 32767)
    {
        return 32767;
    }

    if (value < -32768)
    {
        return (q15_t)-32768;
    }

    return (q15_t)value;
}

static q15_t q15_mul(q15_t left, q15_t right)
{
    return saturate_q15(((int32_t)left * (int32_t)right) >> 15);
}

static void clear_latest_result(void)
{
    m_latestResult.valid = false;
    m_latestResult.frameIndex = 0u;
    m_latestResult.rx = APP_RANGE_FFT_RX_TO_PROCESS;
    m_latestResult.lockedBin = 0u;
    m_latestResult.voteCount = 0u;
    m_latestResult.iAvg = 0;
    m_latestResult.qAvg = 0;
}

static uint32_t compute_chirp_mean(const AppRangeFft_AdcCube_t adc,
                                   uint32_t rx,
                                   uint32_t chirp,
                                   uint16_t *minSample,
                                   uint16_t *maxSample,
                                   uint32_t *clipCount)
{
    uint32_t sum = 0u;

    *minSample = 4095u;
    *maxSample = 0u;

    for (uint32_t index = 0u; index < APP_RANGE_FFT_SAMPLES_PER_CHIRP; index++)
    {
        const uint16_t sample = adc[rx][chirp][index];

        if (sample < *minSample)
        {
            *minSample = sample;
        }

        if (sample > *maxSample)
        {
            *maxSample = sample;
        }

        if ((sample <= APP_RANGE_FFT_CLIP_LOW_THRESHOLD) || (sample >= APP_RANGE_FFT_CLIP_HIGH_THRESHOLD))
        {
            (*clipCount)++;
        }

        sum += sample;
    }

    return sum / APP_RANGE_FFT_SAMPLES_PER_CHIRP;
}

static void prepare_fft_input(const AppRangeFft_AdcCube_t adc, uint32_t rx, uint32_t chirp, uint32_t mean)
{
    for (uint32_t index = 0u; index < APP_RANGE_FFT_SAMPLES_PER_CHIRP; index++)
    {
        const uint16_t rawSample = adc[rx][chirp][index];
        const int32_t centered = (int32_t)rawSample - (int32_t)mean;
        const q15_t q15Sample = saturate_q15(centered << 4);
        const q15_t windowedSample = q15_mul(q15Sample, g_blackmanQ15[index]);

        m_fftBuffer[2u * index] = windowedSample;
        m_fftBuffer[(2u * index) + 1u] = 0;
    }
}

static uint64_t get_matrix_bin_magnitude(uint32_t chirp, uint32_t bin)
{
    const int32_t realValue = m_rangeFftMatrix[chirp][bin].real;
    const int32_t imagValue = m_rangeFftMatrix[chirp][bin].imag;

    return (uint64_t)abs_i32(realValue) + (uint64_t)abs_i32(imagValue);
}

static void process_chirp_fft(const AppRangeFft_AdcCube_t adc,
                              uint32_t rx,
                              uint32_t chirp,
                              uint16_t *rawMin,
                              uint16_t *rawMax,
                              uint32_t *rawMean,
                              uint32_t *clipCount)
{
    uint16_t minSample;
    uint16_t maxSample;
    const uint32_t mean = compute_chirp_mean(adc, rx, chirp, &minSample, &maxSample, clipCount);

    prepare_fft_input(adc, rx, chirp, mean);
    arm_cfft_radix2_q15(&m_fftInstance, m_fftBuffer);

    for (uint32_t bin = 0u; bin < APP_RANGE_FFT_MATRIX_BIN_COUNT; bin++)
    {
        m_rangeFftMatrix[chirp][bin].real = m_fftBuffer[2u * bin];
        m_rangeFftMatrix[chirp][bin].imag = m_fftBuffer[(2u * bin) + 1u];
    }

    if (minSample < *rawMin)
    {
        *rawMin = minSample;
    }

    if (maxSample > *rawMax)
    {
        *rawMax = maxSample;
    }

    *rawMean += mean;
}

static void update_top_vote_bins(uint32_t bin, uint32_t votes, uint64_t energy, uint32_t *topBins, uint32_t *topVotes, uint64_t *topEnergies)
{
    for (uint32_t rank = 0u; rank < APP_RANGE_FFT_TOP_BIN_COUNT; rank++)
    {
        if ((votes > topVotes[rank]) || ((votes == topVotes[rank]) && (energy > topEnergies[rank])))
        {
            for (uint32_t move = APP_RANGE_FFT_TOP_BIN_COUNT - 1u; move > rank; move--)
            {
                topBins[move] = topBins[move - 1u];
                topVotes[move] = topVotes[move - 1u];
                topEnergies[move] = topEnergies[move - 1u];
            }

            topBins[rank] = bin;
            topVotes[rank] = votes;
            topEnergies[rank] = energy;
            break;
        }
    }
}

static uint32_t find_chirp_peak_bin(uint32_t chirp)
{
    uint32_t peakBin = APP_RANGE_FFT_SEARCH_START_BIN;
    uint64_t peakMagnitude = 0u;

    for (uint32_t bin = APP_RANGE_FFT_SEARCH_START_BIN; bin <= APP_RANGE_FFT_SEARCH_END_BIN; bin++)
    {
        const uint64_t magnitude = get_matrix_bin_magnitude(chirp, bin);

        if (magnitude > peakMagnitude)
        {
            peakMagnitude = magnitude;
            peakBin = bin;
        }
    }

    return peakBin;
}

static uint32_t vote_last_chirps(uint32_t *topBins, uint32_t *topVotes, uint64_t *topEnergies)
{
    uint32_t votes[APP_RANGE_FFT_MATRIX_BIN_COUNT];
    uint64_t energies[APP_RANGE_FFT_MATRIX_BIN_COUNT];
    uint32_t bestBin = APP_RANGE_FFT_SEARCH_START_BIN;
    uint32_t bestVotes = 0u;
    uint64_t bestEnergy = 0u;
    const uint32_t firstVoteChirp = APP_RANGE_FFT_CHIRPS_PER_FRAME - APP_RANGE_FFT_VOTE_CHIRP_COUNT;

    for (uint32_t bin = 0u; bin < APP_RANGE_FFT_MATRIX_BIN_COUNT; bin++)
    {
        votes[bin] = 0u;
        energies[bin] = 0u;
    }

    for (uint32_t rank = 0u; rank < APP_RANGE_FFT_TOP_BIN_COUNT; rank++)
    {
        topBins[rank] = APP_RANGE_FFT_SEARCH_START_BIN;
        topVotes[rank] = 0u;
        topEnergies[rank] = 0u;
    }

    for (uint32_t chirp = firstVoteChirp; chirp < APP_RANGE_FFT_CHIRPS_PER_FRAME; chirp++)
    {
        const uint32_t peakBin = find_chirp_peak_bin(chirp);

        votes[peakBin]++;
        energies[peakBin] += get_matrix_bin_magnitude(chirp, peakBin);
    }

    for (uint32_t bin = APP_RANGE_FFT_SEARCH_START_BIN; bin <= APP_RANGE_FFT_SEARCH_END_BIN; bin++)
    {
        if ((votes[bin] > bestVotes) || ((votes[bin] == bestVotes) && (energies[bin] > bestEnergy)))
        {
            bestVotes = votes[bin];
            bestEnergy = energies[bin];
            bestBin = bin;
        }

        update_top_vote_bins(bin, votes[bin], energies[bin], topBins, topVotes, topEnergies);
    }

    return bestBin;
}

static void update_latest_result(uint32_t lockedBin, uint32_t voteCount)
{
    int32_t realSum = 0;
    int32_t imagSum = 0;
    const uint32_t firstVoteChirp = APP_RANGE_FFT_CHIRPS_PER_FRAME - APP_RANGE_FFT_VOTE_CHIRP_COUNT;

    for (uint32_t chirp = firstVoteChirp; chirp < APP_RANGE_FFT_CHIRPS_PER_FRAME; chirp++)
    {
        realSum += m_rangeFftMatrix[chirp][lockedBin].real;
        imagSum += m_rangeFftMatrix[chirp][lockedBin].imag;
    }

    m_latestResult.valid = true;
    m_latestResult.frameIndex = m_frameCounter;
    m_latestResult.rx = APP_RANGE_FFT_RX_TO_PROCESS;
    m_latestResult.lockedBin = (uint8_t)lockedBin;
    m_latestResult.voteCount = (uint8_t)voteCount;
    m_latestResult.iAvg = saturate_q15(realSum / (int32_t)APP_RANGE_FFT_VOTE_CHIRP_COUNT);
    m_latestResult.qAvg = saturate_q15(imagSum / (int32_t)APP_RANGE_FFT_VOTE_CHIRP_COUNT);
}

#if APP_RANGE_FFT_PRINT_RAW_CHIRP_ONLY
static void print_adc_cube_chirp0(const AppRangeFft_AdcCube_t adc)
{
    for (uint32_t rx = 0u; rx < APP_RANGE_FFT_RX_CHANNELS; rx++)
    {
        for (uint32_t offset = 0u; offset < APP_RANGE_FFT_SAMPLES_PER_CHIRP; offset += APP_RANGE_FFT_RAW_PRINT_CHUNK_SIZE)
        {
            (void)BoardOutput_printf("adc64,layout=mode3,rx=%lu,chirp=0,i=%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu\r\n",
                                     (unsigned long)rx,
                                     (unsigned long)offset,
                                     (unsigned long)adc[rx][0u][offset + 0u],
                                     (unsigned long)adc[rx][0u][offset + 1u],
                                     (unsigned long)adc[rx][0u][offset + 2u],
                                     (unsigned long)adc[rx][0u][offset + 3u],
                                     (unsigned long)adc[rx][0u][offset + 4u],
                                     (unsigned long)adc[rx][0u][offset + 5u],
                                     (unsigned long)adc[rx][0u][offset + 6u],
                                     (unsigned long)adc[rx][0u][offset + 7u]);
        }
    }
}
#endif

bool AppRangeFft_convertPayloadToAdcCube(const uint8_t *payload, uint32_t byteCount, AppRangeFft_AdcCube_t adc)
{
    if ((payload == NULL) || (adc == NULL) || (byteCount < APP_RANGE_FFT_FRAME_BYTE_COUNT))
    {
        return false;
    }

    for (uint32_t chirp = 0u; chirp < APP_RANGE_FFT_CHIRPS_PER_FRAME; chirp++)//按照chirp遍历
    {
        const uint32_t chirpByteOffset = chirp * APP_RANGE_FFT_CHIRP_BYTE_COUNT;//计算当前chirp的启示的字节到结束的字节 首个chirp（3通道）有299bytes
        //这里是由于3通道采集的模式，所以每次处理9个数据刚好是每个通道的2个sample，后面的数据可以同样循环处理
        for (uint32_t sample = 0u; sample < APP_RANGE_FFT_SAMPLES_PER_CHIRP; sample += 2u)
        {
            //计算偏移多少个字节根据sample数
            const uint32_t wordOffset = chirpByteOffset + ((sample / 2u) * 9u);
            const uint8_t *word0 = &payload[wordOffset + 0u];
            const uint8_t *word1 = &payload[wordOffset + 3u];
            const uint8_t *word2 = &payload[wordOffset + 6u];

            adc[0u][chirp][sample] = unpack_fifo_word_high12(word0);
            adc[1u][chirp][sample] = unpack_fifo_word_low12(word0);
            adc[2u][chirp][sample] = unpack_fifo_word_high12(word1);
            adc[0u][chirp][sample + 1u] = unpack_fifo_word_low12(word1);
            adc[1u][chirp][sample + 1u] = unpack_fifo_word_high12(word2);
            adc[2u][chirp][sample + 1u] = unpack_fifo_word_low12(word2);
        }
    }

    return true;
}

void AppRangeFft_initialize(void)
{
    if (m_initialized || m_initFailed)
    {
        return;
    }
        //初始化arm_cfft_radix2_instance_q15 fft相关的结构体
    if (arm_cfft_radix2_init_q15(&m_fftInstance, APP_RANGE_FFT_FFT_SIZE, 0u, 1u) != ARM_MATH_SUCCESS)
    {
        m_initFailed = true;
        (void)BoardOutput_printf("fft,error=init\r\n");
        return;
    }

    m_frameCounter = 0u;
    m_pendingFrame = false;
    clear_latest_result();
    m_initialized = true;
}

void AppRangeFft_submitFrame(const uint8_t *packed12Data, uint32_t byteCount, uint8_t channel, uint64_t timestamp)
{
    if ((packed12Data == NULL) || (byteCount < APP_RANGE_FFT_FRAME_BYTE_COUNT))
    {
        return;
    }

    if (BoardOutput_getMode() != BOARD_OUTPUT_MODE_DEBUG_TEXT)
    {
        m_pendingFrame = false;
        return;
    }

    if (m_pendingFrame)
    {
        return;
    }

    memcpy(m_pendingFramePacked12, packed12Data, APP_RANGE_FFT_FRAME_BYTE_COUNT);
    m_pendingChannel = channel;
    m_pendingTimestamp = timestamp;
    m_pendingFrame = true;
}

void AppRangeFft_run(void)
{
    if (BoardOutput_getMode() != BOARD_OUTPUT_MODE_DEBUG_TEXT)
    {
        m_pendingFrame = false;
        return;
    }

    if (!m_pendingFrame)
    {
        return;
    }

    m_pendingFrame = false;
    AppRangeFft_process(m_pendingFramePacked12, APP_RANGE_FFT_FRAME_BYTE_COUNT, m_pendingChannel, m_pendingTimestamp);
}


void AppRangeFft_process(const uint8_t *packed12Data, uint32_t byteCount, uint8_t channel, uint64_t timestamp)
{
    uint16_t rawMin = 4095u;
    uint16_t rawMax = 0u;
    uint32_t rawMean = 0u;
    uint32_t topBins[APP_RANGE_FFT_TOP_BIN_COUNT];
    uint32_t topVotes[APP_RANGE_FFT_TOP_BIN_COUNT];
    uint64_t topEnergies[APP_RANGE_FFT_TOP_BIN_COUNT];
    uint32_t clipCount = 0u;
    uint32_t lockedBin;

    (void)timestamp;
    (void)channel;

    if (!m_initialized)
    {
        AppRangeFft_initialize();
    }

    if (!m_initialized || (packed12Data == NULL) || (byteCount < APP_RANGE_FFT_FRAME_BYTE_COUNT))
    {
        return;
    }

    if (!AppRangeFft_convertPayloadToAdcCube(packed12Data, byteCount, m_adcCube))
    {
        return;
    }

    m_frameCounter++;
    if (m_frameCounter < APP_RANGE_FFT_INIT_DELAY_FRAMES)
    {
        return;
    }

    for (uint32_t chirp = 0u; chirp < APP_RANGE_FFT_CHIRPS_PER_FRAME; chirp++)
    {
        process_chirp_fft(m_adcCube,
                          APP_RANGE_FFT_RX_TO_PROCESS,
                          chirp,
                          &rawMin,
                          &rawMax,
                          &rawMean,
                          &clipCount);
    }

    if ((m_frameCounter % APP_RANGE_FFT_RAW_RATE_DIVISOR) == 1u)
    {
        const uint16_t head0 = m_adcCube[APP_RANGE_FFT_RX_TO_PROCESS][0u][0u];
        const uint16_t head1 = m_adcCube[APP_RANGE_FFT_RX_TO_PROCESS][0u][1u];
        const uint16_t head2 = m_adcCube[APP_RANGE_FFT_RX_TO_PROCESS][0u][2u];
        const uint16_t head3 = m_adcCube[APP_RANGE_FFT_RX_TO_PROCESS][0u][3u];

        (void)BoardOutput_printf("raw,layout=mode3,rx=%u,chirps=%lu,min=%lu,max=%lu,mean=%lu,clip=%lu,head=%lu,%lu,%lu,%lu\r\n",
                                 (unsigned int)APP_RANGE_FFT_RX_TO_PROCESS,
                                 (unsigned long)APP_RANGE_FFT_CHIRPS_PER_FRAME,
                                 (unsigned long)rawMin,
                                 (unsigned long)rawMax,
                                 (unsigned long)(rawMean / APP_RANGE_FFT_CHIRPS_PER_FRAME),
                                 (unsigned long)clipCount,
                                 (unsigned long)head0,
                                 (unsigned long)head1,
                                 (unsigned long)head2,
                                 (unsigned long)head3);
    }

#if APP_RANGE_FFT_PRINT_RAW_CHIRP_ONLY
    print_adc_cube_chirp0(m_adcCube);
    return;
#endif

    lockedBin = vote_last_chirps(topBins, topVotes, topEnergies);
    update_latest_result(lockedBin, topVotes[0]);

    if ((m_frameCounter % APP_RANGE_FFT_DEBUG_RATE_DIVISOR) != 1u)
    {
        return;
    }

    (void)BoardOutput_printf("fft,bin=%lu,rx=%u,vote=%lu/%u,top=%lu:%lu,%lu:%lu,%lu:%lu,iq=%d,%d\r\n",
                             (unsigned long)lockedBin,
                             (unsigned int)APP_RANGE_FFT_RX_TO_PROCESS,
                             (unsigned long)topVotes[0],
                             (unsigned int)APP_RANGE_FFT_VOTE_CHIRP_COUNT,
                             (unsigned long)topBins[0],
                             (unsigned long)topVotes[0],
                             (unsigned long)topBins[1],
                             (unsigned long)topVotes[1],
                             (unsigned long)topBins[2],
                             (unsigned long)topVotes[2],
                             (int)m_latestResult.iAvg,
                             (int)m_latestResult.qAvg);
}

bool AppRangeFft_getLatestResult(AppRangeFft_Result_t *result)
{
    if (result == NULL)
    {
        return false;
    }

    *result = m_latestResult;
    return m_latestResult.valid;
}
