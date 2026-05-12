#ifndef COMMANDS_IPINSLTR11_H_
#define COMMANDS_IPINSLTR11_H_ 1

#include <components/interfaces/IPinsLtr11.h>
#include <stdint.h>


uint8_t Commands_IPinsLtr11_read(IPinsLtr11 *pinsLtr11, uint8_t bFunction, uint16_t wLength, uint8_t **payload);
uint8_t Commands_IPinsLtr11_write(IPinsLtr11 *pinsLtr11, uint8_t bFunction, uint16_t wLength, const uint8_t *payload);
uint8_t Commands_IPinsLtr11_transfer(IPinsLtr11 *pinsLtr11, uint8_t bFunction, uint16_t wLengthIn, const uint8_t *payloadIn, uint16_t *wLengthOut, uint8_t **payloadOut);


#endif /* COMMANDS_IPINSLTR11_H_ */
