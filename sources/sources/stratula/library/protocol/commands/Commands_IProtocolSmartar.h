#ifndef COMMANDS_I_COMMANDS_SMARTAR_H
#define COMMANDS_I_COMMANDS_SMARTAR_H 1

#include <components/interfaces/IProtocolSmartar.h>
#include <stdint.h>


uint8_t Commands_IProtocolSmartar_read(IProtocolSmartar *registers, uint8_t bFunction, uint16_t wLength, uint8_t **payload);
uint8_t Commands_IProtocolSmartar_write(IProtocolSmartar *registers, uint8_t bFunction, uint16_t wLength, const uint8_t *payload);
uint8_t Commands_IProtocolSmartar_transfer(IProtocolSmartar *registers, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut);


#endif /* COMMANDS_I_COMMANDS_SMARTAR_H */
