/**
 *
 */
#ifndef EXTENDED_DATA
#define EXTENDED_DATA 1

#include <platform/interfaces/IData.h>
#include <platform/interfaces/IVendorCommands.h>
#include <stdint.h>


extern IData ExtendedData;

void ExtendedData_Constructor(IVendorCommands *vendorCommands, IData *localData, uint8_t localCount);

sr_t ExtendedData_configure(uint8_t index, const IDataProperties_t *dataProperties, const uint8_t *settings, uint16_t settingsSize);
sr_t ExtendedData_start(uint8_t index);
sr_t ExtendedData_stop(uint8_t index);
sr_t ExtendedData_getStatusFlags(uint8_t index, uint32_t *flags);

#endif /* EXTENDED_DATA */
