#include "AppVitalSigns.h"
#include "BoardOutput.h"

#include <arm_math.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define APP_VITAL_SIGNS_PI                         (3.14159265358979323846f)
#define APP_VITAL_SIGNS_TWO_PI                     (2.0f * APP_VITAL_SIGNS_PI)
#define APP_VITAL_SIGNS_PHASE_JUMP_LIMIT_RAD       (1.0f)
#define APP_VITAL_SIGNS_PHASE_RELOCK_JUMP_COUNT    (4u)
#define APP_VITAL_SIGNS_COMPUTE_PHASE_DIFFERENCE   (0u)
#define APP_VITAL_SIGNS_REMOVE_IMPULSE_NOISE       (0u)
#define APP_VITAL_SIGNS_IMPULSE_NOISE_THRESH_RAD   (1.5f)
#define APP_VITAL_SIGNS_TREND_ALPHA                (0.005f)
#define APP_VITAL_SIGNS_TREND_RETAIN               (1.0f - APP_VITAL_SIGNS_TREND_ALPHA)
#define APP_VITAL_SIGNS_MIN_VOTE_COUNT             (7u)
#define APP_VITAL_SIGNS_EXPECTED_VOTE_COUNT        (10u)
#define APP_VITAL_SIGNS_MIN_IQ_MAG_SQ              (64.0f)
#define APP_VITAL_SIGNS_DEBUG_RATE_DIVISOR         (16u)
#define APP_VITAL_SIGNS_REPRESENTATIVE_CHIRP_INDEX (APP_RANGE_FFT_PHASE_SAMPLE_COUNT - 1u)
#define APP_VITAL_SIGNS_ENABLE_PHASE128_DUMP       (0u)
#define APP_VITAL_SIGNS_ENABLE_BRIEF_OUTPUT        (1u)
#define APP_VITAL_SIGNS_ENABLE_DETAILED_OUTPUT     (0u)
#define APP_VITAL_SIGNS_ENABLE_FRAME_OUTPUT        (0u)
#define APP_VITAL_SIGNS_ENABLE_FFT_OUTPUT          (0u)
#define APP_VITAL_SIGNS_WAVELENGTH_MM              (4.94f)
#define APP_VITAL_SIGNS_DISPLACEMENT_SCALE_UM      ((APP_VITAL_SIGNS_WAVELENGTH_MM * 1000.0f) / (4.0f * APP_VITAL_SIGNS_PI))
#define APP_VITAL_SIGNS_DUMP_VALUES_PER_LINE       (8u)
#define APP_VITAL_SIGNS_DUMP_TYPE_COUNT            (3u)
#define APP_VITAL_SIGNS_DUMP_INVALID_VALUE         (999999L)
#define APP_VITAL_SIGNS_FRAME_QUEUE_COUNT          (64u)
#define APP_VITAL_SIGNS_IIR_COEFS_PER_STAGE        (6u)
#define APP_VITAL_SIGNS_BREATH_IIR_STAGE_COUNT     (2u)
#define APP_VITAL_SIGNS_HEART_IIR_STAGE_COUNT      (4u)
#define APP_VITAL_SIGNS_BREATH_DELAY_COUNT         ((APP_VITAL_SIGNS_IIR_COEFS_PER_STAGE * APP_VITAL_SIGNS_BREATH_IIR_STAGE_COUNT) + 2u)
#define APP_VITAL_SIGNS_HEART_DELAY_COUNT          ((APP_VITAL_SIGNS_IIR_COEFS_PER_STAGE * APP_VITAL_SIGNS_HEART_IIR_STAGE_COUNT) + 2u)
#define APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE     (256u)
#define APP_VITAL_SIGNS_PHASE_FFT_WARMUP_SIZE      (512u)
#define APP_VITAL_SIGNS_PHASE_FFT_SIZE             (1024u)
#define APP_VITAL_SIGNS_SAMPLING_FREQ_MILLIHZ      (12500u)
#define APP_VITAL_SIGNS_BIN_INVALID                (0xFFu)
#define APP_VITAL_SIGNS_BIN_CHANGE_CONFIRM_COUNT   (8u)
#define APP_VITAL_SIGNS_PREVIEW_MIN_OK_COUNT       (192u)
#define APP_VITAL_SIGNS_PREVIEW_BREATH_MIN_QUALITY (115u)
#define APP_VITAL_SIGNS_PREVIEW_HEART_MIN_QUALITY  (115u)
#define APP_VITAL_SIGNS_PREVIEW_HIGH_QUALITY       (200u)
#define APP_VITAL_SIGNS_PREVIEW_BREATH_CONFIRM     (2u)
#define APP_VITAL_SIGNS_PREVIEW_HEART_CONFIRM      (3u)
#define APP_VITAL_SIGNS_BREATH_PREVIEW_MIN_BIN     (3u)
#define APP_VITAL_SIGNS_BREATH_PREVIEW_MAX_BIN     (12u)
#define APP_VITAL_SIGNS_HEART_PREVIEW_MIN_BIN      (17u)
#define APP_VITAL_SIGNS_HEART_PREVIEW_MAX_BIN      (40u)
#define APP_VITAL_SIGNS_BREATH_MIN_BIN             (9u)
#define APP_VITAL_SIGNS_BREATH_MAX_BIN             (49u)
#define APP_VITAL_SIGNS_HEART_MIN_BIN              (66u)
#define APP_VITAL_SIGNS_HEART_MAX_BIN              (163u)
#define APP_VITAL_SIGNS_Q15_FFT_SCALE              (30000)
#define APP_VITAL_SIGNS_BPM10_NUMERATOR            (7500u)

static const float g_breathFilterCoefs[APP_VITAL_SIGNS_BREATH_IIR_STAGE_COUNT * APP_VITAL_SIGNS_IIR_COEFS_PER_STAGE] = {
    1.000000f, 0.000000f, -1.000000f, 1.000000f, -1.936979f, 0.940064f,
    1.000000f, 0.000000f, -1.000000f, 1.000000f, -1.682723f, 0.745584f
};

static const float g_breathScaleVals[APP_VITAL_SIGNS_BREATH_IIR_STAGE_COUNT + 1u] = {
    0.115582f, 0.115582f, 1.000000f
};

static const float g_heartFilterCoefs[APP_VITAL_SIGNS_HEART_IIR_STAGE_COUNT * APP_VITAL_SIGNS_IIR_COEFS_PER_STAGE] = {
    1.000000f, 0.000000f, -1.000000f, 1.000000f, -1.240371f, 0.418455f,
    1.000000f, 0.000000f, -1.000000f, 1.000000f, -1.636008f, 0.785230f,
    1.000000f, 0.000000f, -1.000000f, 1.000000f,  0.232874f, 0.095401f,
    1.000000f, 0.000000f, -1.000000f, 1.000000f,  0.636389f, 0.568938f
};

static const float g_heartScaleVals[APP_VITAL_SIGNS_HEART_IIR_STAGE_COUNT + 1u] = {
    0.564095f, 0.564095f, 0.564095f, 0.564095f, 1.000000f
};

typedef struct
{
    uint32_t frameIndex;
    uint8_t bin;
    uint8_t rx;
    uint8_t voteCount;
    uint16_t phaseSampleIndex;
    uint16_t phaseSampleCount;
    uint16_t validPhaseCount;
    int16_t i;
    int16_t q;
    int32_t phaseMrad;
    int32_t unwrapMrad;
    int32_t detrendMrad;
    int32_t phaseDiffMrad;
    int32_t phaseUsedMrad;
    int32_t breathMrad;
    int32_t heartMrad;
    int32_t displacementUm;
    uint32_t slowTimeCount;
    uint16_t slowBufferCount;
    bool previewBpmValid;
    uint16_t previewOkCount;
    uint16_t previewBreathPeakBin;
    uint16_t previewHeartPeakBin;
    uint16_t previewBreathBpm10;
    uint16_t previewHeartBpm10;
    bool previewStableBpmValid;
    uint16_t previewStableBreathBpm10;
    uint16_t previewStableHeartBpm10;
    uint16_t previewBreathQuality;
    uint16_t previewHeartQuality;
    bool bpmValid;
    uint16_t bpmOkCount;
    uint16_t bpmRealSampleCount;
    bool bpmWarmup;
    bool bpmZeroPadded;
    uint16_t breathPeakBin;
    uint16_t heartPeakBin;
    uint16_t breathBpm10;
    uint16_t heartBpm10;
    AppVitalSigns_BpmSource_t bpmSource;
    AppVitalSigns_Status_t status;
} AppVitalSigns_FrameSummary_t;

static bool m_initialized = false;
static bool m_phaseStateValid = false;
static bool m_lastFrameValid = false;
static uint32_t m_lastProcessedFrameIndex = 0u;
static uint8_t m_activeBin = 0u;
static float m_phasePrev = 0.0f;
static float m_unwrapPhase = 0.0f;
static float m_trend = 0.0f;
static float m_phaseDiffPrevUnwrap = 0.0f;
static bool m_phaseDiffStateValid = false;
static uint8_t m_phaseJumpCount = 0u;
static uint8_t m_candidateBin = APP_VITAL_SIGNS_BIN_INVALID;
static uint8_t m_candidateBinCount = 0u;
static float m_impulsePrev2 = 0.0f;
static float m_impulsePrev1 = 0.0f;
static bool m_impulsePrev2Valid = false;
static bool m_impulsePrev1Valid = false;
static float m_breathDelay[APP_VITAL_SIGNS_BREATH_DELAY_COUNT];
static float m_heartDelay[APP_VITAL_SIGNS_HEART_DELAY_COUNT];
static float m_lastBreathWave = 0.0f;
static float m_lastHeartWave = 0.0f;
static bool m_filterOutputValid = false;
static uint32_t m_slowTimeCount = 0u;
static arm_cfft_radix2_instance_q15 m_slowFftInstance;
static bool m_slowFftInitialized = false;
static bool m_slowFftInitFailed = false;
static arm_cfft_radix2_instance_q15 m_slowFftPreviewInstance;
static bool m_slowFftPreviewInitialized = false;
static bool m_slowFftPreviewInitFailed = false;
static q15_t m_slowFftBuffer[APP_VITAL_SIGNS_PHASE_FFT_SIZE * 2u];
static int32_t m_breathCircularBuffer[APP_VITAL_SIGNS_PHASE_FFT_SIZE];
static int32_t m_heartCircularBuffer[APP_VITAL_SIGNS_PHASE_FFT_SIZE];
static bool m_slowSampleOk[APP_VITAL_SIGNS_PHASE_FFT_SIZE];
static uint16_t m_slowBufferWriteIndex = 0u;
static uint16_t m_slowBufferCount = 0u;
static uint16_t m_slowOkCount = 0u;
static bool m_previewBpmValid = false;
static uint16_t m_previewOkCount = 0u;
static uint16_t m_previewBreathPeakBin = 0u;
static uint16_t m_previewHeartPeakBin = 0u;
static uint16_t m_previewBreathBpm10 = 0u;
static uint16_t m_previewHeartBpm10 = 0u;
static uint32_t m_previewBreathPeakPower = 0u;
static uint32_t m_previewHeartPeakPower = 0u;
static bool m_previewStableBreathValid = false;
static bool m_previewStableHeartValid = false;
static uint16_t m_previewStableBreathBpm10 = 0u;
static uint16_t m_previewStableHeartBpm10 = 0u;
static uint16_t m_previewBreathQuality = 0u;
static uint16_t m_previewHeartQuality = 0u;
static uint16_t m_previewBreathCandidateBpm10 = 0u;
static uint16_t m_previewHeartCandidateBpm10 = 0u;
static uint8_t m_previewBreathCandidateCount = 0u;
static uint8_t m_previewHeartCandidateCount = 0u;
static bool m_bpmValid = false;
static uint16_t m_bpmOkCount = 0u;
static uint16_t m_bpmRealSampleCount = 0u;
static bool m_bpmWarmup = true;
static bool m_bpmZeroPadded = false;
static uint16_t m_breathPeakBin = 0u;
static uint16_t m_heartPeakBin = 0u;
static uint16_t m_breathBpm10 = 0u;
static uint16_t m_heartBpm10 = 0u;
static AppVitalSigns_BpmSource_t m_bpmSource = APP_VITAL_SIGNS_BPM_SOURCE_NONE;
static uint32_t m_breathPeakPower = 0u;
static uint32_t m_heartPeakPower = 0u;
static AppVitalSigns_Result_t m_latestResult;
static int32_t m_phaseDumpMrad[APP_RANGE_FFT_PHASE_SAMPLE_COUNT];
static int32_t m_unwrapDumpMrad[APP_RANGE_FFT_PHASE_SAMPLE_COUNT];
static int32_t m_detrendDumpMrad[APP_RANGE_FFT_PHASE_SAMPLE_COUNT];
static bool m_phaseDumpValid[APP_RANGE_FFT_PHASE_SAMPLE_COUNT];
static bool m_phaseDumpActive = false;
static uint32_t m_phaseDumpFrameIndex = 0u;
static uint8_t m_phaseDumpBin = 0u;
static uint8_t m_phaseDumpTypeIndex = 0u;
static uint16_t m_phaseDumpStartIndex = 0u;
static AppVitalSigns_FrameSummary_t m_frameQueue[APP_VITAL_SIGNS_FRAME_QUEUE_COUNT];
static uint8_t m_frameQueueHead = 0u;
static uint8_t m_frameQueueTail = 0u;
static uint8_t m_frameQueueCount = 0u;

static int32_t float_to_i32(float value)
{
    if (value >= 0.0f)
    {
        return (int32_t)(value + 0.5f);
    }

    return (int32_t)(value - 0.5f);
}

static const char *status_to_text(AppVitalSigns_Status_t status)
{
    switch (status)
    {
        case APP_VITAL_SIGNS_STATUS_OK:
            return "ok";
        case APP_VITAL_SIGNS_STATUS_WAIT_RANGE:
            return "wait_range";
        case APP_VITAL_SIGNS_STATUS_LOW_VOTE:
            return "low_vote";
        case APP_VITAL_SIGNS_STATUS_LOW_IQ:
            return "low_iq";
        case APP_VITAL_SIGNS_STATUS_BIN_CHANGED_RESET:
            return "bin_reset";
        case APP_VITAL_SIGNS_STATUS_BIN_CHANGE_HOLD:
            return "bin_hold_legacy";
        case APP_VITAL_SIGNS_STATUS_BIN_REANCHOR:
            return "bin_reanchor_keep";
        case APP_VITAL_SIGNS_STATUS_PHASE_JUMP_SKIPPED:
            return "phase_jump";
        case APP_VITAL_SIGNS_STATUS_PHASE_RELOCKED:
            return "phase_relock";
        default:
            return "unknown";
    }
}

static const char *bpm_source_to_text(AppVitalSigns_BpmSource_t source)
{
    switch (source)
    {
        case APP_VITAL_SIGNS_BPM_SOURCE_FFT:
            return "fft";
        case APP_VITAL_SIGNS_BPM_SOURCE_HOLD:
            return "hold";
        case APP_VITAL_SIGNS_BPM_SOURCE_NONE:
        default:
            return "none";
    }
}

static void clear_latest_result(AppVitalSigns_Status_t status)
{
    m_latestResult.valid = false;
    m_latestResult.frameIndex = 0u;
    m_latestResult.bin = 0u;
    m_latestResult.rx = 0u;
    m_latestResult.voteCount = 0u;
    m_latestResult.phaseSampleIndex = 0u;
    m_latestResult.phaseSampleCount = 0u;
    m_latestResult.validPhaseCount = 0u;
    m_latestResult.i = 0;
    m_latestResult.q = 0;
    m_latestResult.phaseMrad = 0;
    m_latestResult.unwrapMrad = 0;
    m_latestResult.detrendMrad = 0;
    m_latestResult.phaseDiffMrad = 0;
    m_latestResult.phaseUsedMrad = 0;
    m_latestResult.breathMrad = 0;
    m_latestResult.heartMrad = 0;
    m_latestResult.displacementUm = 0;
    m_latestResult.slowTimeCount = m_slowTimeCount;
    m_latestResult.slowBufferCount = m_slowBufferCount;
    m_latestResult.previewBpmValid = m_previewBpmValid;
    m_latestResult.previewOkCount = m_previewOkCount;
    m_latestResult.previewBreathPeakBin = m_previewBreathPeakBin;
    m_latestResult.previewHeartPeakBin = m_previewHeartPeakBin;
    m_latestResult.previewBreathBpm10 = m_previewBreathBpm10;
    m_latestResult.previewHeartBpm10 = m_previewHeartBpm10;
    m_latestResult.previewStableBpmValid = m_previewStableBreathValid && m_previewStableHeartValid;
    m_latestResult.previewStableBreathBpm10 = m_previewStableBreathBpm10;
    m_latestResult.previewStableHeartBpm10 = m_previewStableHeartBpm10;
    m_latestResult.previewBreathQuality = m_previewBreathQuality;
    m_latestResult.previewHeartQuality = m_previewHeartQuality;
    m_latestResult.bpmValid = m_bpmValid;
    m_latestResult.bpmOkCount = m_bpmOkCount;
    m_latestResult.bpmRealSampleCount = m_bpmRealSampleCount;
    m_latestResult.bpmWarmup = m_bpmWarmup;
    m_latestResult.bpmZeroPadded = m_bpmZeroPadded;
    m_latestResult.breathPeakBin = m_breathPeakBin;
    m_latestResult.heartPeakBin = m_heartPeakBin;
    m_latestResult.breathBpm10 = m_breathBpm10;
    m_latestResult.heartBpm10 = m_heartBpm10;
    m_latestResult.bpmSource = m_bpmSource;
    m_latestResult.status = status;
}

static void clear_phase_dump(void)
{
    m_phaseDumpActive = false;
    m_phaseDumpFrameIndex = 0u;
    m_phaseDumpBin = 0u;
    m_phaseDumpTypeIndex = 0u;
    m_phaseDumpStartIndex = 0u;

    for (uint16_t sampleIndex = 0u; sampleIndex < APP_RANGE_FFT_PHASE_SAMPLE_COUNT; sampleIndex++)
    {
        m_phaseDumpMrad[sampleIndex] = APP_VITAL_SIGNS_DUMP_INVALID_VALUE;
        m_unwrapDumpMrad[sampleIndex] = APP_VITAL_SIGNS_DUMP_INVALID_VALUE;
        m_detrendDumpMrad[sampleIndex] = APP_VITAL_SIGNS_DUMP_INVALID_VALUE;
        m_phaseDumpValid[sampleIndex] = false;
    }
}

static void clear_frame_queue(void)
{
    m_frameQueueHead = 0u;
    m_frameQueueTail = 0u;
    m_frameQueueCount = 0u;
}

static void clear_slow_time_buffers(void)
{
    for (uint16_t index = 0u; index < APP_VITAL_SIGNS_PHASE_FFT_SIZE; index++)
    {
        m_breathCircularBuffer[index] = 0;
        m_heartCircularBuffer[index] = 0;
        m_slowSampleOk[index] = false;
    }

    m_slowBufferWriteIndex = 0u;
    m_slowBufferCount = 0u;
    m_slowOkCount = 0u;
    m_previewBpmValid = false;
    m_previewOkCount = 0u;
    m_previewBreathPeakBin = 0u;
    m_previewHeartPeakBin = 0u;
    m_previewBreathBpm10 = 0u;
    m_previewHeartBpm10 = 0u;
    m_previewBreathPeakPower = 0u;
    m_previewHeartPeakPower = 0u;
    m_previewStableBreathValid = false;
    m_previewStableHeartValid = false;
    m_previewStableBreathBpm10 = 0u;
    m_previewStableHeartBpm10 = 0u;
    m_previewBreathQuality = 0u;
    m_previewHeartQuality = 0u;
    m_previewBreathCandidateBpm10 = 0u;
    m_previewHeartCandidateBpm10 = 0u;
    m_previewBreathCandidateCount = 0u;
    m_previewHeartCandidateCount = 0u;
    m_bpmValid = false;
    m_bpmOkCount = 0u;
    m_bpmRealSampleCount = 0u;
    m_bpmWarmup = true;
    m_bpmZeroPadded = false;
    m_breathPeakBin = 0u;
    m_heartPeakBin = 0u;
    m_breathBpm10 = 0u;
    m_heartBpm10 = 0u;
    m_bpmSource = APP_VITAL_SIGNS_BPM_SOURCE_NONE;
    m_breathPeakPower = 0u;
    m_heartPeakPower = 0u;
}

static bool initialize_slow_time_fft(arm_cfft_radix2_instance_q15 *instance,
                                     bool *initialized,
                                     bool *initFailed,
                                     uint16_t fftSize)
{
    if (*initialized)
    {
        return true;
    }

    if (*initFailed)
    {
        return false;
    }

    if (arm_cfft_radix2_init_q15(instance, fftSize, 0u, 1u) != ARM_MATH_SUCCESS)
    {
        *initFailed = true;
        return false;
    }

    *initialized = true;
    return true;
}

static void enqueue_frame_summary_from_latest(void)
{
    AppVitalSigns_FrameSummary_t *summary;

    if (m_frameQueueCount >= APP_VITAL_SIGNS_FRAME_QUEUE_COUNT)
    {
        m_frameQueueTail = (uint8_t)((m_frameQueueTail + 1u) % APP_VITAL_SIGNS_FRAME_QUEUE_COUNT);
        m_frameQueueCount--;
    }

    summary = &m_frameQueue[m_frameQueueHead];
    summary->frameIndex = m_latestResult.frameIndex;
    summary->bin = m_latestResult.bin;
    summary->rx = m_latestResult.rx;
    summary->voteCount = m_latestResult.voteCount;
    summary->phaseSampleIndex = m_latestResult.phaseSampleIndex;
    summary->phaseSampleCount = m_latestResult.phaseSampleCount;
    summary->validPhaseCount = m_latestResult.validPhaseCount;
    summary->i = m_latestResult.i;
    summary->q = m_latestResult.q;
    summary->phaseMrad = m_latestResult.phaseMrad;
    summary->unwrapMrad = m_latestResult.unwrapMrad;
    summary->detrendMrad = m_latestResult.detrendMrad;
    summary->phaseDiffMrad = m_latestResult.phaseDiffMrad;
    summary->phaseUsedMrad = m_latestResult.phaseUsedMrad;
    summary->breathMrad = m_latestResult.breathMrad;
    summary->heartMrad = m_latestResult.heartMrad;
    summary->displacementUm = m_latestResult.displacementUm;
    summary->slowTimeCount = m_latestResult.slowTimeCount;
    summary->slowBufferCount = m_latestResult.slowBufferCount;
    summary->previewBpmValid = m_latestResult.previewBpmValid;
    summary->previewOkCount = m_latestResult.previewOkCount;
    summary->previewBreathPeakBin = m_latestResult.previewBreathPeakBin;
    summary->previewHeartPeakBin = m_latestResult.previewHeartPeakBin;
    summary->previewBreathBpm10 = m_latestResult.previewBreathBpm10;
    summary->previewHeartBpm10 = m_latestResult.previewHeartBpm10;
    summary->previewStableBpmValid = m_latestResult.previewStableBpmValid;
    summary->previewStableBreathBpm10 = m_latestResult.previewStableBreathBpm10;
    summary->previewStableHeartBpm10 = m_latestResult.previewStableHeartBpm10;
    summary->previewBreathQuality = m_latestResult.previewBreathQuality;
    summary->previewHeartQuality = m_latestResult.previewHeartQuality;
    summary->bpmValid = m_latestResult.bpmValid;
    summary->bpmOkCount = m_latestResult.bpmOkCount;
    summary->bpmRealSampleCount = m_latestResult.bpmRealSampleCount;
    summary->bpmWarmup = m_latestResult.bpmWarmup;
    summary->bpmZeroPadded = m_latestResult.bpmZeroPadded;
    summary->breathPeakBin = m_latestResult.breathPeakBin;
    summary->heartPeakBin = m_latestResult.heartPeakBin;
    summary->breathBpm10 = m_latestResult.breathBpm10;
    summary->heartBpm10 = m_latestResult.heartBpm10;
    summary->bpmSource = m_latestResult.bpmSource;
    summary->status = m_latestResult.status;

    m_frameQueueHead = (uint8_t)((m_frameQueueHead + 1u) % APP_VITAL_SIGNS_FRAME_QUEUE_COUNT);
    m_frameQueueCount++;
}

static void reset_phase_state(void)
{
    m_phaseStateValid = false;
    m_activeBin = 0u;
    m_phasePrev = 0.0f;
    m_unwrapPhase = 0.0f;
    m_trend = 0.0f;
    m_phaseDiffPrevUnwrap = 0.0f;
    m_phaseDiffStateValid = false;
    m_phaseJumpCount = 0u;
    m_candidateBin = APP_VITAL_SIGNS_BIN_INVALID;
    m_candidateBinCount = 0u;
    m_impulsePrev2 = 0.0f;
    m_impulsePrev1 = 0.0f;
    m_impulsePrev2Valid = false;
    m_impulsePrev1Valid = false;
    m_lastBreathWave = 0.0f;
    m_lastHeartWave = 0.0f;
    m_filterOutputValid = false;
    for (uint32_t index = 0u; index < APP_VITAL_SIGNS_BREATH_DELAY_COUNT; index++)
    {
        m_breathDelay[index] = 0.0f;
    }
    for (uint32_t index = 0u; index < APP_VITAL_SIGNS_HEART_DELAY_COUNT; index++)
    {
        m_heartDelay[index] = 0.0f;
    }
    m_slowTimeCount = 0u;
    clear_slow_time_buffers();
}

static void clear_phase_history_after_relock(void)
{
    m_phaseDiffPrevUnwrap = m_unwrapPhase;
    m_phaseDiffStateValid = false;
    m_impulsePrev2 = 0.0f;
    m_impulsePrev1 = 0.0f;
    m_impulsePrev2Valid = false;
    m_impulsePrev1Valid = false;
}

static void reanchor_phase_state_keep_slow_buffer(uint8_t lockedBin, float phase)
{
    m_phaseStateValid = true;
    m_activeBin = lockedBin;
    m_phasePrev = phase;
    m_phaseJumpCount = 0u;
    m_candidateBin = APP_VITAL_SIGNS_BIN_INVALID;
    m_candidateBinCount = 0u;
    clear_phase_history_after_relock();
}

static void clear_bin_change_candidate(void)
{
    m_candidateBin = APP_VITAL_SIGNS_BIN_INVALID;
    m_candidateBinCount = 0u;
}

static void set_result_from_phase(const AppRangeFft_Result_t *rangeResult,
                                  uint16_t phaseSampleIndex,
                                  uint16_t validPhaseCount,
                                  int16_t iValue,
                                  int16_t qValue,
                                  float phase,
                                  float unwrapPhase,
                                  float detrendedPhase,
                                  float phaseDiff,
                                  float phaseUsed,
                                  float breathWave,
                                  float heartWave,
                                  AppVitalSigns_Status_t status)
{
    m_latestResult.valid = true;
    m_latestResult.frameIndex = rangeResult->frameIndex;
    m_latestResult.bin = rangeResult->lockedBin;
    m_latestResult.rx = rangeResult->rx;
    m_latestResult.voteCount = rangeResult->voteCount;
    m_latestResult.phaseSampleIndex = phaseSampleIndex;
    m_latestResult.phaseSampleCount = rangeResult->phaseSampleCount;
    m_latestResult.validPhaseCount = validPhaseCount;
    m_latestResult.i = iValue;
    m_latestResult.q = qValue;
    m_latestResult.phaseMrad = float_to_i32(phase * 1000.0f);
    m_latestResult.unwrapMrad = float_to_i32(unwrapPhase * 1000.0f);
    m_latestResult.detrendMrad = float_to_i32(detrendedPhase * 1000.0f);
    m_latestResult.phaseDiffMrad = float_to_i32(phaseDiff * 1000.0f);
    m_latestResult.phaseUsedMrad = float_to_i32(phaseUsed * 1000.0f);
    m_latestResult.breathMrad = float_to_i32(breathWave * 1000.0f);
    m_latestResult.heartMrad = float_to_i32(heartWave * 1000.0f);
    m_latestResult.displacementUm = float_to_i32(detrendedPhase * APP_VITAL_SIGNS_DISPLACEMENT_SCALE_UM);
    m_latestResult.slowTimeCount = m_slowTimeCount;
    m_latestResult.slowBufferCount = m_slowBufferCount;
    m_latestResult.previewBpmValid = m_previewBpmValid;
    m_latestResult.previewOkCount = m_previewOkCount;
    m_latestResult.previewBreathPeakBin = m_previewBreathPeakBin;
    m_latestResult.previewHeartPeakBin = m_previewHeartPeakBin;
    m_latestResult.previewBreathBpm10 = m_previewBreathBpm10;
    m_latestResult.previewHeartBpm10 = m_previewHeartBpm10;
    m_latestResult.previewStableBpmValid = m_previewStableBreathValid && m_previewStableHeartValid;
    m_latestResult.previewStableBreathBpm10 = m_previewStableBreathBpm10;
    m_latestResult.previewStableHeartBpm10 = m_previewStableHeartBpm10;
    m_latestResult.previewBreathQuality = m_previewBreathQuality;
    m_latestResult.previewHeartQuality = m_previewHeartQuality;
    m_latestResult.bpmValid = m_bpmValid;
    m_latestResult.bpmOkCount = m_bpmOkCount;
    m_latestResult.bpmRealSampleCount = m_bpmRealSampleCount;
    m_latestResult.bpmWarmup = m_bpmWarmup;
    m_latestResult.bpmZeroPadded = m_bpmZeroPadded;
    m_latestResult.breathPeakBin = m_breathPeakBin;
    m_latestResult.heartPeakBin = m_heartPeakBin;
    m_latestResult.breathBpm10 = m_breathBpm10;
    m_latestResult.heartBpm10 = m_heartBpm10;
    m_latestResult.bpmSource = m_bpmSource;
    m_latestResult.status = status;
}

static float filter_iir_biquad_cascade(float dataIn,
                                       const float *filterCoefs,
                                       const float *scaleVals,
                                       float *delay,
                                       uint16_t numStages)
{
    float input = dataIn;
    float output = 0.0f;

    for (uint16_t stage = 0u; stage < numStages; stage++)
    {
        const uint16_t index = (uint16_t)(APP_VITAL_SIGNS_IIR_COEFS_PER_STAGE * stage);
        const float b0 = filterCoefs[index + 0u];
        const float b1 = filterCoefs[index + 1u];
        const float b2 = filterCoefs[index + 2u];
        const float a1 = filterCoefs[index + 4u];
        const float a2 = filterCoefs[index + 5u];

        delay[index] = (scaleVals[stage] * input) - (a1 * delay[index + 1u]) - (a2 * delay[index + 2u]);
        output = (b0 * delay[index]) + (b1 * delay[index + 1u]) + (b2 * delay[index + 2u]);

        delay[index + 2u] = delay[index + 1u];
        delay[index + 1u] = delay[index];
        input = output;
    }

    return output;
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

static uint16_t bpm10_from_fft_bin(uint16_t bin, uint16_t fftSize)
{
    return (uint16_t)(((uint32_t)bin * APP_VITAL_SIGNS_BPM10_NUMERATOR +
                       (fftSize / 2u)) /
                      fftSize);
}

static uint16_t abs_diff_u16(uint16_t left, uint16_t right)
{
    return (left > right) ? (uint16_t)(left - right) : (uint16_t)(right - left);
}

static uint16_t preview_bpm_step10(void)
{
    return bpm10_from_fft_bin(1u, APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE);
}

static uint16_t move_bpm_toward(uint16_t current, uint16_t target, uint16_t maxStep)
{
    if (target > current)
    {
        const uint16_t delta = (uint16_t)(target - current);
        return (uint16_t)(current + ((delta > maxStep) ? maxStep : delta));
    }

    {
        const uint16_t delta = (uint16_t)(current - target);
        return (uint16_t)(current - ((delta > maxStep) ? maxStep : delta));
    }
}

static uint16_t peak_quality_from_powers(uint32_t bestPower, uint32_t secondPower)
{
    uint64_t quality;

    if (bestPower == 0u)
    {
        return 0u;
    }

    if (secondPower == 0u)
    {
        return 9999u;
    }

    quality = (((uint64_t)bestPower * 100u) + (secondPower / 2u)) / secondPower;
    return (quality > 9999u) ? 9999u : (uint16_t)quality;
}

static int32_t get_slow_time_sample(const int32_t *buffer, uint16_t fftSize, uint16_t chronologicalIndex)
{
    const uint16_t startIndex = (uint16_t)((m_slowBufferWriteIndex +
                                            APP_VITAL_SIGNS_PHASE_FFT_SIZE -
                                            fftSize) %
                                           APP_VITAL_SIGNS_PHASE_FFT_SIZE);
    const uint16_t bufferIndex = (uint16_t)((startIndex + chronologicalIndex) %
                                           APP_VITAL_SIGNS_PHASE_FFT_SIZE);
    return buffer[bufferIndex];
}

static void prepare_slow_time_fft_input(const int32_t *buffer, uint16_t fftSize, uint16_t inputSampleCount)
{
    int64_t sum = 0;
    int32_t mean;
    int32_t maxAbs = 1;
    uint16_t sampleCount = inputSampleCount;

    if (sampleCount > fftSize)
    {
        sampleCount = fftSize;
    }

    for (uint16_t index = 0u; index < fftSize; index++)
    {
        m_slowFftBuffer[2u * index] = 0;
        m_slowFftBuffer[(2u * index) + 1u] = 0;
    }

    if (sampleCount == 0u)
    {
        return;
    }

    for (uint16_t index = 0u; index < sampleCount; index++)
    {
        sum += get_slow_time_sample(buffer, sampleCount, index);
    }

    mean = (int32_t)(sum / (int64_t)sampleCount);

    for (uint16_t index = 0u; index < sampleCount; index++)
    {
        const int32_t centered = get_slow_time_sample(buffer, sampleCount, index) - mean;
        const int32_t magnitude = abs_i32(centered);

        if (magnitude > maxAbs)
        {
            maxAbs = magnitude;
        }
    }

    for (uint16_t index = 0u; index < sampleCount; index++)
    {
        const int32_t centered = get_slow_time_sample(buffer, sampleCount, index) - mean;
        const int32_t scaled = (int32_t)(((int64_t)centered * APP_VITAL_SIGNS_Q15_FFT_SCALE) / maxAbs);

        m_slowFftBuffer[2u * index] = saturate_q15(scaled);
        m_slowFftBuffer[(2u * index) + 1u] = 0;
    }
}

static uint32_t get_slow_time_fft_power(uint16_t bin)
{
    const int32_t realValue = m_slowFftBuffer[2u * bin];
    const int32_t imagValue = m_slowFftBuffer[(2u * bin) + 1u];

    return (uint32_t)((realValue * realValue) + (imagValue * imagValue));
}

static uint16_t count_recent_ok_samples(uint16_t sampleCount)
{
    uint16_t okCount = 0u;
    const uint16_t startIndex = (uint16_t)((m_slowBufferWriteIndex +
                                            APP_VITAL_SIGNS_PHASE_FFT_SIZE -
                                            sampleCount) %
                                           APP_VITAL_SIGNS_PHASE_FFT_SIZE);

    for (uint16_t index = 0u; index < sampleCount; index++)
    {
        const uint16_t bufferIndex = (uint16_t)((startIndex + index) %
                                               APP_VITAL_SIGNS_PHASE_FFT_SIZE);
        if (m_slowSampleOk[bufferIndex])
        {
            okCount++;
        }
    }

    return okCount;
}

static bool compute_slow_time_fft_peak(const int32_t *buffer,
                                       uint16_t fftSize,
                                       uint16_t inputSampleCount,
                                       uint16_t minBin,
                                       uint16_t maxBin,
                                       uint16_t *peakBin,
                                       uint16_t *bpm10,
                                       uint32_t *peakPower,
                                       uint16_t *peakQuality,
                                       arm_cfft_radix2_instance_q15 *instance,
                                       bool *initialized,
                                       bool *initFailed)
{
    uint32_t bestPower = 0u;
    uint32_t secondPower = 0u;
    uint16_t bestBin = minBin;
    uint16_t sampleCount = inputSampleCount;

    if (sampleCount > fftSize)
    {
        sampleCount = fftSize;
    }

    if ((fftSize == 0u) || (sampleCount == 0u) || (maxBin >= (fftSize / 2u)))
    {
        return false;
    }

    if (!initialize_slow_time_fft(instance, initialized, initFailed, fftSize))
    {
        return false;
    }

    prepare_slow_time_fft_input(buffer, fftSize, sampleCount);
    arm_cfft_radix2_q15(instance, m_slowFftBuffer);

    for (uint16_t bin = minBin; bin <= maxBin; bin++)
    {
        const uint32_t power = get_slow_time_fft_power(bin);

        if (power > bestPower)
        {
            secondPower = bestPower;
            bestPower = power;
            bestBin = bin;
        }
        else if (power > secondPower)
        {
            secondPower = power;
        }
    }

    *peakBin = bestBin;
    *bpm10 = bpm10_from_fft_bin(bestBin, fftSize);
    *peakPower = bestPower;
    *peakQuality = peak_quality_from_powers(bestPower, secondPower);
    return true;
}

static void update_preview_stable_estimate(uint16_t rawBpm10,
                                           uint16_t quality,
                                           uint16_t minQuality,
                                           uint8_t confirmCount,
                                           uint16_t *candidateBpm10,
                                           uint8_t *candidateCount,
                                           uint16_t *stableBpm10,
                                           bool *stableValid)
{
    const uint16_t maxStep = preview_bpm_step10();
    const bool usablePeak = (m_previewOkCount >= APP_VITAL_SIGNS_PREVIEW_MIN_OK_COUNT) &&
                            (quality >= minQuality);

    if (!usablePeak)
    {
        return;
    }

    if (!*stableValid)
    {
        *stableBpm10 = rawBpm10;
        *stableValid = true;
        *candidateBpm10 = rawBpm10;
        *candidateCount = 1u;
        return;
    }

    if (abs_diff_u16(rawBpm10, *candidateBpm10) <= maxStep)
    {
        if (*candidateCount < confirmCount)
        {
            (*candidateCount)++;
        }
    }
    else
    {
        *candidateBpm10 = rawBpm10;
        *candidateCount = 1u;
    }

    if (quality >= APP_VITAL_SIGNS_PREVIEW_HIGH_QUALITY)
    {
        *candidateCount = confirmCount;
    }

    if (abs_diff_u16(rawBpm10, *stableBpm10) <= maxStep)
    {
        *stableBpm10 = rawBpm10;
    }
    else if (*candidateCount >= confirmCount)
    {
        *stableBpm10 = move_bpm_toward(*stableBpm10, rawBpm10, maxStep);
    }
}

static void update_bpm_estimates(void)
{
    bool previewBreathOk;
    bool previewHeartOk;
    bool breathOk;
    bool heartOk;
    uint16_t breathQuality = 0u;
    uint16_t heartQuality = 0u;

    if (m_slowBufferCount >= APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE)
    {
        m_previewOkCount = count_recent_ok_samples(APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE);
        previewBreathOk = compute_slow_time_fft_peak(m_breathCircularBuffer,
                                                     APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE,
                                                     APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE,
                                                     APP_VITAL_SIGNS_BREATH_PREVIEW_MIN_BIN,
                                                     APP_VITAL_SIGNS_BREATH_PREVIEW_MAX_BIN,
                                                     &m_previewBreathPeakBin,
                                                     &m_previewBreathBpm10,
                                                     &m_previewBreathPeakPower,
                                                     &m_previewBreathQuality,
                                                     &m_slowFftPreviewInstance,
                                                     &m_slowFftPreviewInitialized,
                                                     &m_slowFftPreviewInitFailed);
        previewHeartOk = compute_slow_time_fft_peak(m_heartCircularBuffer,
                                                    APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE,
                                                    APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE,
                                                    APP_VITAL_SIGNS_HEART_PREVIEW_MIN_BIN,
                                                    APP_VITAL_SIGNS_HEART_PREVIEW_MAX_BIN,
                                                    &m_previewHeartPeakBin,
                                                    &m_previewHeartBpm10,
                                                    &m_previewHeartPeakPower,
                                                    &m_previewHeartQuality,
                                                    &m_slowFftPreviewInstance,
                                                    &m_slowFftPreviewInitialized,
                                                    &m_slowFftPreviewInitFailed);
        m_previewBpmValid = previewBreathOk && previewHeartOk;
        if (previewBreathOk)
        {
            update_preview_stable_estimate(m_previewBreathBpm10,
                                           m_previewBreathQuality,
                                           APP_VITAL_SIGNS_PREVIEW_BREATH_MIN_QUALITY,
                                           APP_VITAL_SIGNS_PREVIEW_BREATH_CONFIRM,
                                           &m_previewBreathCandidateBpm10,
                                           &m_previewBreathCandidateCount,
                                           &m_previewStableBreathBpm10,
                                           &m_previewStableBreathValid);
        }
        if (previewHeartOk)
        {
            update_preview_stable_estimate(m_previewHeartBpm10,
                                           m_previewHeartQuality,
                                           APP_VITAL_SIGNS_PREVIEW_HEART_MIN_QUALITY,
                                           APP_VITAL_SIGNS_PREVIEW_HEART_CONFIRM,
                                           &m_previewHeartCandidateBpm10,
                                           &m_previewHeartCandidateCount,
                                           &m_previewStableHeartBpm10,
                                           &m_previewStableHeartValid);
        }
    }
    else
    {
        m_previewBpmValid = false;
        m_previewOkCount = 0u;
        m_previewBreathQuality = 0u;
        m_previewHeartQuality = 0u;
    }

    if (m_slowBufferCount < APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE)
    {
        m_bpmValid = false;
        m_bpmOkCount = 0u;
        m_bpmRealSampleCount = m_slowBufferCount;
        m_bpmWarmup = true;
        m_bpmZeroPadded = true;
        m_bpmSource = APP_VITAL_SIGNS_BPM_SOURCE_NONE;
        return;
    }

    if (m_slowBufferCount >= APP_VITAL_SIGNS_PHASE_FFT_WARMUP_SIZE)
    {
        m_bpmRealSampleCount = APP_VITAL_SIGNS_PHASE_FFT_WARMUP_SIZE;
    }
    else
    {
        m_bpmRealSampleCount = APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE;
    }

    m_bpmOkCount = count_recent_ok_samples(m_bpmRealSampleCount);
    m_bpmWarmup = (m_slowBufferCount < APP_VITAL_SIGNS_PHASE_FFT_WARMUP_SIZE);
    m_bpmZeroPadded = (m_bpmRealSampleCount < APP_VITAL_SIGNS_PHASE_FFT_SIZE);

    if (m_bpmWarmup && (m_bpmOkCount < APP_VITAL_SIGNS_PREVIEW_MIN_OK_COUNT))
    {
        m_bpmValid = false;
        m_bpmSource = APP_VITAL_SIGNS_BPM_SOURCE_NONE;
        return;
    }

    breathOk = compute_slow_time_fft_peak(m_breathCircularBuffer,
                                          APP_VITAL_SIGNS_PHASE_FFT_SIZE,
                                          m_bpmRealSampleCount,
                                          APP_VITAL_SIGNS_BREATH_MIN_BIN,
                                          APP_VITAL_SIGNS_BREATH_MAX_BIN,
                                          &m_breathPeakBin,
                                          &m_breathBpm10,
                                          &m_breathPeakPower,
                                          &breathQuality,
                                          &m_slowFftInstance,
                                          &m_slowFftInitialized,
                                          &m_slowFftInitFailed);
    heartOk = compute_slow_time_fft_peak(m_heartCircularBuffer,
                                         APP_VITAL_SIGNS_PHASE_FFT_SIZE,
                                         m_bpmRealSampleCount,
                                         APP_VITAL_SIGNS_HEART_MIN_BIN,
                                         APP_VITAL_SIGNS_HEART_MAX_BIN,
                                         &m_heartPeakBin,
                                         &m_heartBpm10,
                                         &m_heartPeakPower,
                                         &heartQuality,
                                         &m_slowFftInstance,
                                         &m_slowFftInitialized,
                                         &m_slowFftInitFailed);
    m_bpmValid = breathOk && heartOk;
    m_bpmSource = m_bpmValid ? APP_VITAL_SIGNS_BPM_SOURCE_FFT : APP_VITAL_SIGNS_BPM_SOURCE_NONE;
}

static void push_slow_time_sample(float breathWave, float heartWave, bool realSample)
{
    if ((m_slowBufferCount >= APP_VITAL_SIGNS_PHASE_FFT_SIZE) &&
        m_slowSampleOk[m_slowBufferWriteIndex] &&
        (m_slowOkCount > 0u))
    {
        m_slowOkCount--;
    }

    m_breathCircularBuffer[m_slowBufferWriteIndex] = float_to_i32(breathWave * 1000.0f);
    m_heartCircularBuffer[m_slowBufferWriteIndex] = float_to_i32(heartWave * 1000.0f);
    m_slowSampleOk[m_slowBufferWriteIndex] = realSample;
    if (realSample && (m_slowOkCount < APP_VITAL_SIGNS_PHASE_FFT_SIZE))
    {
        m_slowOkCount++;
    }

    m_slowBufferWriteIndex = (uint16_t)((m_slowBufferWriteIndex + 1u) % APP_VITAL_SIGNS_PHASE_FFT_SIZE);
    if (m_slowBufferCount < APP_VITAL_SIGNS_PHASE_FFT_SIZE)
    {
        m_slowBufferCount++;
    }

    m_slowTimeCount++;
    update_bpm_estimates();
}

static void update_vital_filters(float phaseUsed, float *breathWave, float *heartWave)
{
    *breathWave = filter_iir_biquad_cascade(phaseUsed,
                                            g_breathFilterCoefs,
                                            g_breathScaleVals,
                                            m_breathDelay,
                                            APP_VITAL_SIGNS_BREATH_IIR_STAGE_COUNT);
    *heartWave = filter_iir_biquad_cascade(phaseUsed,
                                           g_heartFilterCoefs,
                                           g_heartScaleVals,
                                           m_heartDelay,
                                           APP_VITAL_SIGNS_HEART_IIR_STAGE_COUNT);
    m_lastBreathWave = *breathWave;
    m_lastHeartWave = *heartWave;
    m_filterOutputValid = true;
    push_slow_time_sample(*breathWave, *heartWave, true);
}

static void hold_vital_filter_outputs(float *breathWave, float *heartWave)
{
    if (!m_filterOutputValid)
    {
        return;
    }

    *breathWave = m_lastBreathWave;
    *heartWave = m_lastHeartWave;
    push_slow_time_sample(*breathWave, *heartWave, false);
}

#if APP_VITAL_SIGNS_REMOVE_IMPULSE_NOISE
static float remove_impulse_noise(float dataPrev2, float dataPrev1, float dataCurr, float threshold)
{
    const float backwardDiff = dataPrev1 - dataPrev2;
    const float forwardDiff = dataPrev1 - dataCurr;

    if (((forwardDiff > threshold) && (backwardDiff > threshold)) ||
        ((forwardDiff < -threshold) && (backwardDiff < -threshold)))
    {
        return (dataPrev2 + dataCurr) * 0.5f;
    }

    return dataPrev1;
}
#endif

static float update_phase_used(float phaseInput, float *phaseDiff)
{
    float phaseUsed;

#if APP_VITAL_SIGNS_COMPUTE_PHASE_DIFFERENCE
    if (!m_phaseDiffStateValid)
    {
        *phaseDiff = 0.0f;
        m_phaseDiffPrevUnwrap = phaseInput;
        m_phaseDiffStateValid = true;
    }
    else
    {
        *phaseDiff = phaseInput - m_phaseDiffPrevUnwrap;
        m_phaseDiffPrevUnwrap = phaseInput;
    }
    phaseUsed = *phaseDiff;
#else
    *phaseDiff = 0.0f;
    phaseUsed = phaseInput;
#endif

#if APP_VITAL_SIGNS_REMOVE_IMPULSE_NOISE
    if (m_impulsePrev2Valid && m_impulsePrev1Valid)
    {
        const float rawPhaseUsed = phaseUsed;

        phaseUsed = remove_impulse_noise(m_impulsePrev2,
                                         m_impulsePrev1,
                                         rawPhaseUsed,
                                         APP_VITAL_SIGNS_IMPULSE_NOISE_THRESH_RAD);
        m_impulsePrev2 = m_impulsePrev1;
        m_impulsePrev1 = rawPhaseUsed;
    }
    else if (m_impulsePrev1Valid)
    {
        m_impulsePrev2 = m_impulsePrev1;
        m_impulsePrev2Valid = true;
        m_impulsePrev1 = phaseUsed;
    }
    else
    {
        m_impulsePrev1 = phaseUsed;
        m_impulsePrev1Valid = true;
    }
#endif

    return phaseUsed;
}

static void start_phase_dump_snapshot(uint32_t frameIndex, uint8_t bin)
{
    m_phaseDumpActive = true;
    m_phaseDumpFrameIndex = frameIndex;
    m_phaseDumpBin = bin;
    m_phaseDumpTypeIndex = 0u;
    m_phaseDumpStartIndex = 0u;
}

static int32_t get_dump_value(const int32_t *values, uint16_t sampleIndex)
{
    if (!m_phaseDumpValid[sampleIndex])
    {
        return APP_VITAL_SIGNS_DUMP_INVALID_VALUE;
    }

    return values[sampleIndex];
}

static const char *phase_dump_type_text(uint8_t typeIndex)
{
    switch (typeIndex)
    {
        case 0u:
            return "phase";
        case 1u:
            return "unwrap";
        case 2u:
            return "detrend";
        default:
            return "unknown";
    }
}

static const int32_t *phase_dump_values(uint8_t typeIndex)
{
    switch (typeIndex)
    {
        case 0u:
            return m_phaseDumpMrad;
        case 1u:
            return m_unwrapDumpMrad;
        case 2u:
            return m_detrendDumpMrad;
        default:
            return m_phaseDumpMrad;
    }
}

static sr_t print_phase_dump_line(const char *type, const int32_t *values, uint16_t startIndex)
{
    return BoardOutput_printf("phase128,f=%lu,bin=%u,type=%s,i=%u,%ld,%ld,%ld,%ld,%ld,%ld,%ld,%ld\r\n",
                              (unsigned long)m_phaseDumpFrameIndex,
                              (unsigned int)m_phaseDumpBin,
                              type,
                              (unsigned int)startIndex,
                              (long)get_dump_value(values, startIndex + 0u),
                              (long)get_dump_value(values, startIndex + 1u),
                              (long)get_dump_value(values, startIndex + 2u),
                              (long)get_dump_value(values, startIndex + 3u),
                              (long)get_dump_value(values, startIndex + 4u),
                              (long)get_dump_value(values, startIndex + 5u),
                              (long)get_dump_value(values, startIndex + 6u),
                              (long)get_dump_value(values, startIndex + 7u));
}

static void print_phase_dump(void)
{
    const int32_t *values;
    sr_t result;

    if (!m_phaseDumpActive)
    {
        return;
    }

    values = phase_dump_values(m_phaseDumpTypeIndex);
    result = print_phase_dump_line(phase_dump_type_text(m_phaseDumpTypeIndex), values, m_phaseDumpStartIndex);
    if (result != E_SUCCESS)
    {
        return;
    }

    m_phaseDumpStartIndex += APP_VITAL_SIGNS_DUMP_VALUES_PER_LINE;
    if (m_phaseDumpStartIndex >= APP_RANGE_FFT_PHASE_SAMPLE_COUNT)
    {
        m_phaseDumpStartIndex = 0u;
        m_phaseDumpTypeIndex++;
    }

    if (m_phaseDumpTypeIndex >= APP_VITAL_SIGNS_DUMP_TYPE_COUNT)
    {
        m_phaseDumpActive = false;
    }
}

#if APP_VITAL_SIGNS_ENABLE_FRAME_OUTPUT
static bool print_frame_summary(void)
{
    const AppVitalSigns_FrameSummary_t *summary;
    sr_t result;

    if (m_frameQueueCount == 0u)
    {
        return false;
    }

    summary = &m_frameQueue[m_frameQueueTail];
    result = BoardOutput_printf("vital_frame,f=%lu,bin=%u,rx=%u,vote=%u/%u,slow=%lu,buf=%u/%u,chirp=%u/%u,valid=%u/%u,status=%s,phase=%ld,unwrap=%ld,detrend=%ld,diff=%ld,used=%ld,breath=%ld,heart=%ld,disp=%ld,iq=%d,%d\r\n",
                                (unsigned long)summary->frameIndex,
                                (unsigned int)summary->bin,
                                (unsigned int)summary->rx,
                                (unsigned int)summary->voteCount,
                                (unsigned int)APP_VITAL_SIGNS_EXPECTED_VOTE_COUNT,
                                (unsigned long)summary->slowTimeCount,
                                (unsigned int)summary->slowBufferCount,
                                (unsigned int)APP_VITAL_SIGNS_PHASE_FFT_SIZE,
                                (unsigned int)summary->phaseSampleIndex,
                                (unsigned int)summary->phaseSampleCount,
                                (unsigned int)summary->validPhaseCount,
                                (unsigned int)summary->phaseSampleCount,
                                status_to_text(summary->status),
                                (long)summary->phaseMrad,
                                (long)summary->unwrapMrad,
                                (long)summary->detrendMrad,
                                (long)summary->phaseDiffMrad,
                                (long)summary->phaseUsedMrad,
                                (long)summary->breathMrad,
                                (long)summary->heartMrad,
                                (long)summary->displacementUm,
                                (int)summary->i,
                                (int)summary->q);
    if (result != E_SUCCESS)
    {
        return false;
    }

    m_frameQueueTail = (uint8_t)((m_frameQueueTail + 1u) % APP_VITAL_SIGNS_FRAME_QUEUE_COUNT);
    m_frameQueueCount--;
    return true;
}
#endif

static uint16_t display_bpm10(bool valid, uint16_t bpm10)
{
    return valid ? bpm10 : 0u;
}

static void print_latest_result(void)
{
#if APP_VITAL_SIGNS_ENABLE_DETAILED_OUTPUT
    bool printDetailedResult;
#endif

    if (!m_latestResult.valid)
    {
        return;
    }

#if APP_VITAL_SIGNS_ENABLE_BRIEF_OUTPUT
    {
        const uint16_t breathBpm10 = display_bpm10(m_latestResult.bpmValid, m_latestResult.breathBpm10);
        const uint16_t heartBpm10 = display_bpm10(m_latestResult.bpmValid, m_latestResult.heartBpm10);
        const uint16_t previewBreathBpm10 = display_bpm10(m_latestResult.previewBpmValid, m_latestResult.previewBreathBpm10);
        const uint16_t previewHeartBpm10 = display_bpm10(m_latestResult.previewBpmValid, m_latestResult.previewHeartBpm10);

        (void)BoardOutput_printf("vital_brief,f=%lu,bin=%u,vote=%u/%u,status=%s,src=%s,slow=%lu,ok=%u/%u,br=%u.%u,hr=%u.%u,br256=%u.%u,hr256=%u.%u,br_bin=%u,hr_bin=%u,br_bin256=%u,hr_bin256=%u\r\n",
                                 (unsigned long)m_latestResult.frameIndex,
                                 (unsigned int)m_latestResult.bin,
                                 (unsigned int)m_latestResult.voteCount,
                                 (unsigned int)APP_VITAL_SIGNS_EXPECTED_VOTE_COUNT,
                                 status_to_text(m_latestResult.status),
                                 bpm_source_to_text(m_latestResult.bpmSource),
                                 (unsigned long)m_latestResult.slowTimeCount,
                                 (unsigned int)m_latestResult.bpmOkCount,
                                 (unsigned int)m_latestResult.bpmRealSampleCount,
                                 (unsigned int)(breathBpm10 / 10u),
                                 (unsigned int)(breathBpm10 % 10u),
                                 (unsigned int)(heartBpm10 / 10u),
                                 (unsigned int)(heartBpm10 % 10u),
                                 (unsigned int)(previewBreathBpm10 / 10u),
                                 (unsigned int)(previewBreathBpm10 % 10u),
                                 (unsigned int)(previewHeartBpm10 / 10u),
                                 (unsigned int)(previewHeartBpm10 % 10u),
                                 (unsigned int)m_latestResult.breathPeakBin,
                                 (unsigned int)m_latestResult.heartPeakBin,
                                 (unsigned int)m_latestResult.previewBreathPeakBin,
                                 (unsigned int)m_latestResult.previewHeartPeakBin);
    }
#endif

#if APP_VITAL_SIGNS_ENABLE_DETAILED_OUTPUT
    printDetailedResult = ((m_latestResult.frameIndex % APP_VITAL_SIGNS_DEBUG_RATE_DIVISOR) == 1u);
    if (!printDetailedResult)
    {
        if (!m_latestResult.bpmValid && !m_latestResult.previewBpmValid)
        {
            return;
        }
    }

    if (printDetailedResult)
    {
        (void)BoardOutput_printf("vital,f=%lu,bin=%u,rx=%u,vote=%u/%u,status=%s,phase_i=%u/%u,valid=%u,slow=%lu,buf=%u/%u,phase_mrad=%ld,unwrap_mrad=%ld,detrend_mrad=%ld,diff_mrad=%ld,used_mrad=%ld,breath_mrad=%ld,heart_mrad=%ld,disp_um=%ld\r\n",
                                 (unsigned long)m_latestResult.frameIndex,
                                 (unsigned int)m_latestResult.bin,
                                 (unsigned int)m_latestResult.rx,
                                 (unsigned int)m_latestResult.voteCount,
                                 (unsigned int)APP_VITAL_SIGNS_EXPECTED_VOTE_COUNT,
                                 status_to_text(m_latestResult.status),
                                 (unsigned int)m_latestResult.phaseSampleIndex,
                                 (unsigned int)m_latestResult.phaseSampleCount,
                                 (unsigned int)m_latestResult.validPhaseCount,
                                 (unsigned long)m_latestResult.slowTimeCount,
                                 (unsigned int)m_latestResult.slowBufferCount,
                                 (unsigned int)APP_VITAL_SIGNS_PHASE_FFT_SIZE,
                                 (long)m_latestResult.phaseMrad,
                                 (long)m_latestResult.unwrapMrad,
                                 (long)m_latestResult.detrendMrad,
                                 (long)m_latestResult.phaseDiffMrad,
                                 (long)m_latestResult.phaseUsedMrad,
                                 (long)m_latestResult.breathMrad,
                                 (long)m_latestResult.heartMrad,
                                 (long)m_latestResult.displacementUm);
    }
#endif

#if APP_VITAL_SIGNS_ENABLE_FFT_OUTPUT
    if (m_latestResult.bpmValid)
    {
        (void)BoardOutput_printf("vital_fft1024,f=%lu,slow=%lu,warmup=%u,zp=%u,win=%u/%u,real=%u/%u,total=%u/%u,ok=%u/%u,stable=1,br_bin=%u,br_bpm10=%u,br_est_bpm10=%u,br_pow=%lu,hr_bin=%u,hr_bpm10=%u,hr_est_bpm10=%u,hr_pow=%lu\r\n",
                                 (unsigned long)m_latestResult.frameIndex,
                                 (unsigned long)m_latestResult.slowTimeCount,
                                 m_latestResult.bpmWarmup ? 1u : 0u,
                                 m_latestResult.bpmZeroPadded ? 1u : 0u,
                                 (unsigned int)APP_VITAL_SIGNS_PHASE_FFT_SIZE,
                                 (unsigned int)APP_VITAL_SIGNS_PHASE_FFT_SIZE,
                                 (unsigned int)m_latestResult.bpmRealSampleCount,
                                 (unsigned int)APP_VITAL_SIGNS_PHASE_FFT_SIZE,
                                 (unsigned int)m_latestResult.slowBufferCount,
                                 (unsigned int)APP_VITAL_SIGNS_PHASE_FFT_SIZE,
                                 (unsigned int)m_latestResult.bpmOkCount,
                                 (unsigned int)m_latestResult.bpmRealSampleCount,
                                 (unsigned int)m_latestResult.breathPeakBin,
                                 (unsigned int)m_latestResult.breathBpm10,
                                 (unsigned int)m_latestResult.breathBpm10,
                                 (unsigned long)m_breathPeakPower,
                                 (unsigned int)m_latestResult.heartPeakBin,
                                 (unsigned int)m_latestResult.heartBpm10,
                                 (unsigned int)m_latestResult.heartBpm10,
                                 (unsigned long)m_heartPeakPower);
    }
    if (m_latestResult.previewBpmValid)
    {
        (void)BoardOutput_printf("vital_fft256,f=%lu,slow=%lu,warmup=1,win=%u/%u,total=%u/%u,ok=%u/%u,stable=%u,br_bin=%u,br_bpm10=%u,br_est_bpm10=%u,br_q=%u,br_pow=%lu,hr_bin=%u,hr_bpm10=%u,hr_est_bpm10=%u,hr_q=%u,hr_pow=%lu\r\n",
                                 (unsigned long)m_latestResult.frameIndex,
                                 (unsigned long)m_latestResult.slowTimeCount,
                                 (unsigned int)APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE,
                                 (unsigned int)APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE,
                                 (unsigned int)m_latestResult.slowBufferCount,
                                 (unsigned int)APP_VITAL_SIGNS_PHASE_FFT_SIZE,
                                 (unsigned int)m_latestResult.previewOkCount,
                                 (unsigned int)APP_VITAL_SIGNS_PHASE_FFT_PREVIEW_SIZE,
                                 m_latestResult.previewStableBpmValid ? 1u : 0u,
                                 (unsigned int)m_latestResult.previewBreathPeakBin,
                                 (unsigned int)m_latestResult.previewBreathBpm10,
                                 (unsigned int)m_latestResult.previewStableBreathBpm10,
                                 (unsigned int)m_latestResult.previewBreathQuality,
                                 (unsigned long)m_previewBreathPeakPower,
                                 (unsigned int)m_latestResult.previewHeartPeakBin,
                                 (unsigned int)m_latestResult.previewHeartBpm10,
                                 (unsigned int)m_latestResult.previewStableHeartBpm10,
                                 (unsigned int)m_latestResult.previewHeartQuality,
                                 (unsigned long)m_previewHeartPeakPower);
    }
#endif

}

void AppVitalSigns_initialize(void)
{
    if (m_initialized)
    {
        return;
    }

    m_lastFrameValid = false;
    m_lastProcessedFrameIndex = 0u;
    reset_phase_state();
    clear_latest_result(APP_VITAL_SIGNS_STATUS_WAIT_RANGE);
    clear_frame_queue();
    m_initialized = true;
}

void AppVitalSigns_reset(void)
{
    AppVitalSigns_initialize();
    m_lastFrameValid = false;
    m_lastProcessedFrameIndex = 0u;
    reset_phase_state();
    clear_phase_dump();
    clear_frame_queue();
    clear_latest_result(APP_VITAL_SIGNS_STATUS_WAIT_RANGE);
}

void AppVitalSigns_processFrame(const AppRangeFft_Result_t *rangeResult)
{
    AppVitalSigns_Status_t frameStatus = APP_VITAL_SIGNS_STATUS_OK;
    uint16_t phaseSampleCount;
    uint16_t validPhaseCount = 0u;
    const uint16_t representativeIndex = (uint16_t)APP_VITAL_SIGNS_REPRESENTATIVE_CHIRP_INDEX;
    int16_t iSample = 0;
    int16_t qSample = 0;
    float phase = 0.0f;
    float detrendedPhase = 0.0f;
    float phaseDiff = 0.0f;
    float phaseUsed = 0.0f;
    float breathWave = 0.0f;
    float heartWave = 0.0f;
    bool capturePhaseDump;

    AppVitalSigns_initialize();

    if ((rangeResult == NULL) || !rangeResult->valid)
    {
        clear_latest_result(APP_VITAL_SIGNS_STATUS_WAIT_RANGE);
        return;
    }

    phaseSampleCount = rangeResult->phaseSampleCount;
    if (phaseSampleCount > APP_RANGE_FFT_PHASE_SAMPLE_COUNT)
    {
        phaseSampleCount = APP_RANGE_FFT_PHASE_SAMPLE_COUNT;
    }

    capturePhaseDump = ((APP_VITAL_SIGNS_ENABLE_PHASE128_DUMP != 0u) &&
                        ((rangeResult->frameIndex % APP_VITAL_SIGNS_DEBUG_RATE_DIVISOR) == 1u));
    if (capturePhaseDump)
    {
        clear_phase_dump();
    }

    if (phaseSampleCount <= representativeIndex)
    {
        clear_latest_result(APP_VITAL_SIGNS_STATUS_LOW_IQ);
        m_latestResult.frameIndex = rangeResult->frameIndex;
        m_latestResult.bin = rangeResult->lockedBin;
        m_latestResult.rx = rangeResult->rx;
        m_latestResult.voteCount = rangeResult->voteCount;
        m_latestResult.phaseSampleIndex = representativeIndex;
        m_latestResult.phaseSampleCount = phaseSampleCount;
        m_latestResult.i = rangeResult->iAvg;
        m_latestResult.q = rangeResult->qAvg;
        enqueue_frame_summary_from_latest();
        return;
    }

    iSample = rangeResult->i[representativeIndex];
    qSample = rangeResult->q[representativeIndex];

    {
        const float iValue = (float)iSample;
        const float qValue = (float)qSample;
        const float iqMagnitudeSq = (iValue * iValue) + (qValue * qValue);

        if (iqMagnitudeSq < APP_VITAL_SIGNS_MIN_IQ_MAG_SQ)
        {
            m_phaseJumpCount = 0u;
            clear_latest_result(APP_VITAL_SIGNS_STATUS_LOW_IQ);
            m_latestResult.frameIndex = rangeResult->frameIndex;
            m_latestResult.bin = rangeResult->lockedBin;
            m_latestResult.rx = rangeResult->rx;
            m_latestResult.voteCount = rangeResult->voteCount;
            m_latestResult.phaseSampleIndex = representativeIndex;
            m_latestResult.phaseSampleCount = phaseSampleCount;
            m_latestResult.i = iSample;
            m_latestResult.q = qSample;
            enqueue_frame_summary_from_latest();
            return;
        }

        phase = atan2f(qValue, iValue);
        validPhaseCount++;

        if (m_phaseStateValid && (rangeResult->lockedBin != m_activeBin))
        {
            reanchor_phase_state_keep_slow_buffer(rangeResult->lockedBin, phase);
            frameStatus = APP_VITAL_SIGNS_STATUS_BIN_REANCHOR;
        }
        else
        {
            clear_bin_change_candidate();
        }

        if (!m_phaseStateValid)
        {
            m_phaseStateValid = true;
            m_activeBin = rangeResult->lockedBin;
            m_phasePrev = phase;
            m_unwrapPhase = 0.0f;
            m_trend = 0.0f;
            m_phaseDiffPrevUnwrap = m_unwrapPhase;
            m_phaseDiffStateValid = true;
            m_phaseJumpCount = 0u;
            if (frameStatus != APP_VITAL_SIGNS_STATUS_BIN_REANCHOR)
            {
                frameStatus = APP_VITAL_SIGNS_STATUS_BIN_CHANGED_RESET;
            }
            if (capturePhaseDump)
            {
                m_phaseDumpMrad[representativeIndex] = float_to_i32(phase * 1000.0f);
                m_unwrapDumpMrad[representativeIndex] = 0;
                m_detrendDumpMrad[representativeIndex] = 0;
                m_phaseDumpValid[representativeIndex] = true;
            }
            set_result_from_phase(rangeResult,
                                  representativeIndex,
                                  validPhaseCount,
                                  iSample,
                                  qSample,
                                  phase,
                                  m_unwrapPhase,
                                  0.0f,
                                  phaseDiff,
                                  phaseUsed,
                                  breathWave,
                                  heartWave,
                                  frameStatus);
            if (capturePhaseDump)
            {
                start_phase_dump_snapshot(rangeResult->frameIndex, rangeResult->lockedBin);
            }
            enqueue_frame_summary_from_latest();
            return;
        }

        {
            float diffPhase = phase - m_phasePrev;

            if (diffPhase > APP_VITAL_SIGNS_PI)
            {
                diffPhase -= APP_VITAL_SIGNS_TWO_PI;
            }
            else if (diffPhase < -APP_VITAL_SIGNS_PI)
            {
                diffPhase += APP_VITAL_SIGNS_TWO_PI;
            }

            if (fabsf(diffPhase) > APP_VITAL_SIGNS_PHASE_JUMP_LIMIT_RAD)
            {
                if (m_phaseJumpCount < APP_VITAL_SIGNS_PHASE_RELOCK_JUMP_COUNT)
                {
                    m_phaseJumpCount++;
                }

                if (m_phaseJumpCount >= APP_VITAL_SIGNS_PHASE_RELOCK_JUMP_COUNT)
                {
                    m_phasePrev = phase;
                    m_phaseJumpCount = 0u;
                    clear_phase_history_after_relock();
                    frameStatus = APP_VITAL_SIGNS_STATUS_PHASE_RELOCKED;
                }
                else
                {
                    frameStatus = APP_VITAL_SIGNS_STATUS_PHASE_JUMP_SKIPPED;
                }
            }
            else
            {
                m_unwrapPhase += diffPhase;
                m_phasePrev = phase;
                m_phaseJumpCount = 0u;
            }
        }

        if (frameStatus == APP_VITAL_SIGNS_STATUS_OK)
        {
            m_trend = (APP_VITAL_SIGNS_TREND_RETAIN * m_trend) + (APP_VITAL_SIGNS_TREND_ALPHA * m_unwrapPhase);
        }
        detrendedPhase = m_unwrapPhase - m_trend;
        if (capturePhaseDump)
        {
            m_phaseDumpMrad[representativeIndex] = float_to_i32(phase * 1000.0f);
            m_unwrapDumpMrad[representativeIndex] = float_to_i32(m_unwrapPhase * 1000.0f);
            m_detrendDumpMrad[representativeIndex] = float_to_i32(detrendedPhase * 1000.0f);
            m_phaseDumpValid[representativeIndex] = true;
        }

        if ((frameStatus == APP_VITAL_SIGNS_STATUS_OK) ||
            (frameStatus == APP_VITAL_SIGNS_STATUS_BIN_REANCHOR))
        {
            phaseUsed = update_phase_used(detrendedPhase, &phaseDiff);
            update_vital_filters(phaseUsed, &breathWave, &heartWave);
        }
        else if ((frameStatus == APP_VITAL_SIGNS_STATUS_PHASE_JUMP_SKIPPED) ||
                 (frameStatus == APP_VITAL_SIGNS_STATUS_PHASE_RELOCKED))
        {
            hold_vital_filter_outputs(&breathWave, &heartWave);
        }
        set_result_from_phase(rangeResult,
                              representativeIndex,
                              validPhaseCount,
                              iSample,
                              qSample,
                              phase,
                              m_unwrapPhase,
                              detrendedPhase,
                              phaseDiff,
                              phaseUsed,
                              breathWave,
                              heartWave,
                              frameStatus);
    }

    if (capturePhaseDump)
    {
        start_phase_dump_snapshot(rangeResult->frameIndex, rangeResult->lockedBin);
    }
    enqueue_frame_summary_from_latest();
}

void AppVitalSigns_run(void)
{
    AppRangeFft_Result_t rangeResult;
#if APP_VITAL_SIGNS_ENABLE_FRAME_OUTPUT
    bool printedFrameSummary;
#endif

    AppVitalSigns_initialize();

    if (BoardOutput_getMode() != BOARD_OUTPUT_MODE_DEBUG_TEXT)
    {
        AppVitalSigns_reset();
        return;
    }

#if APP_VITAL_SIGNS_ENABLE_FRAME_OUTPUT
    printedFrameSummary = print_frame_summary();
    if (!printedFrameSummary)
#endif
    {
        print_phase_dump();
    }

    if (!AppRangeFft_getLatestResult(&rangeResult))
    {
        return;
    }

    if (m_lastFrameValid && (rangeResult.frameIndex == m_lastProcessedFrameIndex))
    {
        return;
    }

    m_lastFrameValid = true;
    m_lastProcessedFrameIndex = rangeResult.frameIndex;
    AppVitalSigns_processFrame(&rangeResult);
    print_latest_result();
#if APP_VITAL_SIGNS_ENABLE_FRAME_OUTPUT
    if (!printedFrameSummary)
#endif
    {
#if APP_VITAL_SIGNS_ENABLE_FRAME_OUTPUT
        printedFrameSummary = print_frame_summary();
        if (!printedFrameSummary)
#endif
        {
            print_phase_dump();
        }
    }
}

bool AppVitalSigns_getLatestResult(AppVitalSigns_Result_t *result)
{
    if (result == NULL)
    {
        return false;
    }

    AppVitalSigns_initialize();
    *result = m_latestResult;
    return m_latestResult.valid;
}
