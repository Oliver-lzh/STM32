#ifndef COMMANDS_I_PINS_AVIAN_H_
#define COMMANDS_I_PINS_AVIAN_H_ 1

#include <components/interfaces/IPinsAvian.h>
#include <stdint.h>


uint8_t Commands_IPinsAvian_read(IPinsAvian *pinsAvian, uint8_t bFunction, uint16_t wLength, uint8_t **payload);
uint8_t Commands_IPinsAvian_write(IPinsAvian *pinsAvian, uint8_t bFunction, uint16_t wLength, const uint8_t *payload);
uint8_t Commands_IPinsAvian_transfer(IPinsAvian *pinsAvian, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut);


#endif /* COMMANDS_I_PINS_AVIAN_H_ */
