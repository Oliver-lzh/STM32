/**
 * \addtogroup      IRadar
 * \brief
 * Radar component access interface
 * @{
 */
#ifndef BOARD_RADAR_DEFINITION_T_H
#define BOARD_RADAR_DEFINITION_T_H 1

#include <stdint.h>
#include <universal/data_definitions.h>

typedef struct
{
    uint8_t devId;            ///< control interface device id
    uint8_t dataIndex;        ///< data interface index
    uint8_t channelSwapping;  ///< swapping mode for receive channels
} BoardRadarDefinition_t;

#endif /* BOARD_RADAR_DEFINITION_T_H */

/** @} */
