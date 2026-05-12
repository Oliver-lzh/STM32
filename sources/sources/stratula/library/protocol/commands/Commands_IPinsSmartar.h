#ifndef COMMANDS_I_PINS_SMARTAR_H_
#define COMMANDS_I_PINS_SMARTAR_H_ 1

#include <components/interfaces/IPinsSmartar.h>
#include <stdint.h>


uint8_t Commands_IPinsSmartar_read(IPinsSmartar *pinsSmartar, uint8_t bFunction, uint16_t wLength, uint8_t **payload);
uint8_t Commands_IPinsSmartar_write(IPinsSmartar *pinsSmartar, uint8_t bFunction, uint16_t wLength, const uint8_t *payload);
uint8_t Commands_IPinsSmartar_transfer(IPinsSmartar *pinsSmartar, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut);


#endif /* COMMANDS_I_PINS_SMARTAR_H_ */
