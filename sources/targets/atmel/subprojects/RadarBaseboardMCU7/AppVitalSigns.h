#ifndef APP_VITAL_SIGNS_H_
#define APP_VITAL_SIGNS_H_ 1

#include "AppRangeFft.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    APP_VITAL_SIGNS_STATUS_OK = 0,
    APP_VITAL_SIGNS_STATUS_WAIT_RANGE,
    APP_VITAL_SIGNS_STATUS_LOW_VOTE,
    APP_VITAL_SIGNS_STATUS_LOW_IQ,
    APP_VITAL_SIGNS_STATUS_BIN_CHANGED_RESET,
    APP_VITAL_SIGNS_STATUS_BIN_CHANGE_HOLD,
    APP_VITAL_SIGNS_STATUS_BIN_REANCHOR,
    APP_VITAL_SIGNS_STATUS_PHASE_JUMP_SKIPPED,
    APP_VITAL_SIGNS_STATUS_PHASE_RELOCKED,
} AppVitalSigns_Status_t;

typedef enum
{
    APP_VITAL_SIGNS_BPM_SOURCE_NONE = 0,
    APP_VITAL_SIGNS_BPM_SOURCE_FFT,
} AppVitalSigns_BpmSource_t;

typedef struct
{
    bool valid;
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
    uint16_t bpmRealSampleCount;
    bool bpmWarmup;
    bool bpmZeroPadded;
    uint16_t bpmOkCount;
    uint16_t breathPeakBin;
    uint16_t heartPeakBin;
    uint16_t breathBpm10;
    uint16_t heartBpm10;
    AppVitalSigns_BpmSource_t bpmSource;
    AppVitalSigns_Status_t status;
} AppVitalSigns_Result_t;

void AppVitalSigns_initialize(void);
void AppVitalSigns_reset(void);
void AppVitalSigns_run(void);
void AppVitalSigns_processFrame(const AppRangeFft_Result_t *rangeResult);
bool AppVitalSigns_getLatestResult(AppVitalSigns_Result_t *result);

#endif /* APP_VITAL_SIGNS_H_ */
