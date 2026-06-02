#ifndef APP_RADAR_DEBUG_ACQUISITION_H_
#define APP_RADAR_DEBUG_ACQUISITION_H_ 1

#include <common/errors.h>
#include <components/interfaces/IRadarAvian.h>
#include <platform/interfaces/IData.h>
#include <stdbool.h>
#include <stdint.h>

void AppRadarDebugAcquisition_Constructor(IRadarAvian *radar, IData *data, uint8_t dataIndex);
void AppRadarDebugAcquisition_run(void);
void AppRadarDebugAcquisition_onFrame(uint32_t count, uint8_t channel, uint64_t timestamp);
sr_t AppRadarDebugAcquisition_stop(void);
bool AppRadarDebugAcquisition_isRunning(void);

#endif /* APP_RADAR_DEBUG_ACQUISITION_H_ */
