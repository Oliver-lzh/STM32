#ifndef COMMANDSIRADARLTR11_H_
#define COMMANDSIRADARLTR11_H_ 1

#include <components/interfaces/IRadarLtr11.h>
#include <protocol/commands/ICommands.h>


/** Register a component
 *  @param instance the instance to register
 *  @return true if successful, false if all available slots are occupied
 */
bool Commands_IRadarLtr11_register(IRadarLtr11 *component);


#endif /* COMMANDSIRADARLTR11_H_ */
