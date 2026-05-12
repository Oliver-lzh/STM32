#ifndef _COMMANDS_IPROCESSING_RADAR_H_
#define _COMMANDS_IPROCESSING_RADAR_H_ 1

#include <platform/interfaces/IProcessingRadar.h>
#include <protocol/commands/ICommands.h>


/** Register a radar processing instance
 *  @param instance the instance to register
 *  @return true if successful, false if all available slots are occupied
 */
bool Commands_IProcessingRadar_register(IProcessingRadar *instance);


#endif /* _COMMANDS_IPROCESSING_RADAR_H_ */
