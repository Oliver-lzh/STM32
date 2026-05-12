#ifndef COMMANDS_IRADAR_SMARTAR_H_
#define COMMANDS_IRADAR_SMARTAR_H_ 1

#include <components/interfaces/IRadarSmartar.h>
#include <protocol/commands/ICommands.h>


/** Register a component
 *  @param instance the instance to register
 *  @return true if successful, false if all available slots are occupied
 */
bool Commands_IRadarSmartar_register(IRadarSmartar *instance);


#endif /* COMMANDS_IRADAR_SMARTAR_H_ */
