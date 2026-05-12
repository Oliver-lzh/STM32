#ifndef COMMANDS_IRADAR_AVIAN_H_
#define COMMANDS_IRADAR_AVIAN_H_ 1

#include <components/interfaces/IRadarAvian.h>
#include <protocol/commands/ICommands.h>


/** Register a component
 *  @param instance the instance to register
 *  @return true if successful, false if all available slots are occupied
 */
bool Commands_IRadarAvian_register(IRadarAvian *instance);


#endif /* COMMANDS_IRADAR_AVIAN_H_ */
