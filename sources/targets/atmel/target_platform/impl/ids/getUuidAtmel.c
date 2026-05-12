#include "getUuidAtmel.h"

#include <flash_efc.h>
#include <string.h>


sr_t getUuidAtmel(uint8_t uuid[UUID_LENGTH])
{
    uint32_t ul_rc;
    uint32_t uid_buf[4];

    cpu_irq_enter_critical();
    ul_rc = efc_perform_read_sequence(EFC, EFC_FCMD_STUI, EFC_FCMD_SPUI, uid_buf, 4);
    cpu_irq_leave_critical();

    if (ul_rc != FLASH_RC_OK)
    {
        return E_FAILED;
    }

    memcpy(uuid, uid_buf, UUID_LENGTH);

    return E_SUCCESS;
}
