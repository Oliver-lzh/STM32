
#ifndef DATA_SMARTAR_H_
#define DATA_SMARTAR_H_ 1

#include <components/interfaces/IProtocolSmartar.h>
#include <impl/PlatformInterruptDefinition.h>
#include <platform/interfaces/IData.h>


extern IData DataSmartar;


void DataSmartar_Constructor(IData_acquisitionStatusCallback statusCb);
void DataSmartar_initialize(uint8_t index, uint8_t devId, const PlatformInterruptDefinition_t *irq);
void DataSmartar_setBuffer(uint8_t index, uint16_t *buffer, uint32_t bufferSize);
void DataSmartar_run(void);


sr_t DataSmartar_configure(uint8_t index, const IDataProperties_t *dataProperties, const uint8_t *settings, uint16_t settingsSize);
sr_t DataSmartar_start(uint8_t index);
sr_t DataSmartar_stop(uint8_t index);
sr_t DataSmartar_getStatusFlags(uint8_t index, uint32_t *flags);
sr_t DataSmartar_registerCallback(IData_callback callback, void *arg);


#endif /* DATA_SMARTAR_H_ */
