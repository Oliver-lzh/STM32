/**
 * \internal
 * \addtogroup      Registers
 * \brief
 * Base implementation of device registers access
 * \endinternal
 * @{
 */
#ifndef REGISTERS_I2C_8_H
#define REGISTERS_I2C_8_H 1


#include <components/Registers8.h>
#include <platform/interfaces/II2c.h>


typedef struct
{
    Registers8 b_Registers;

    II2c *m_accessI2c;
    uint16_t m_devAddr;
} RegistersI2c8;


void RegistersI2c8_Constructor(RegistersI2c8 *self, II2c *accessI2c, uint16_t devAddr);


#endif /* REGISTERS_I2C_8_H */

/** @} */
