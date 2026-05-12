#ifndef COMMANDS_IRADAR_ATR22_H_
#define COMMANDS_IRADAR_ATR22_H_ 1

#include <components/interfaces/IRadarAtr22.h>
#include <protocol/commands/ICommands.h>


/** Register a component
 *  @param instance the instance to register
 *  @return true if successful, false if all available slots are occupied
 */
bool Commands_IRadarAtr22_register(IRadarAtr22 *instance);


#endif /* COMMANDS_IRADAR_ATR22_H_ */
