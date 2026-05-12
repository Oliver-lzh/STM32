/* ===========================================================================
** Copyright (C) 2021 Infineon Technologies AG
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice,
**    this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. Neither the name of the copyright holder nor the names of its
**    contributors may be used to endorse or promote products derived from
**    this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
** IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
** ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
** LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
** CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
** SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
** INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
** ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
** POSSIBILITY OF SUCH DAMAGE.
** ===========================================================================
*/

#include <BoardDefinition.h>

#include "timer.h"

#include "peripherals/SysTimer.h"
#include "peripherals/System.h"

#include <impl/Platform.h>
#include <impl/PlatformData.h>
#include <impl/PlatformGpio.h>
#include <impl/PlatformI2c.h>
#include <impl/PlatformSpi.h>

#include <bsp/i2c.h>
#include <bsp/spi.h>

#include <common/typeutils.h>
#include <fatal_error.h>


void PlatformInterfaces_swapI2cIds(void)
{
    if (ARRAY_SIZE_CHECKED(BoardI2cDefinition) != 2)
    {
        fatal_error(0);
    }

    PlatformI2cDefinition_t tmpI2c = BoardI2cDefinition[0];
    BoardI2cDefinition[0]          = BoardI2cDefinition[1];
    BoardI2cDefinition[1]          = tmpI2c;
}

static void PlatformInterfaces_Constructor(void)
{
    // Initialize low-level platform interfaces
    PlatformGpio_initialize(NULL, 0);
    PlatformI2c_initialize(BoardI2cDefinition, ARRAY_SIZE(BoardI2cDefinition));
    PlatformSpi_initialize(BoardSpiDefinition, ARRAY_SIZE(BoardSpiDefinition));
    PlatformData_initialize(NULL, 0);
}

void Platform_Constructor(void)
{
    System_init();
    System_disableWatchdog();

    SysTimer_init();

    System_startUSB();

    PlatformInterfaces_Constructor();

    Timer_Constructor();
}

void Platform_run(void)
{
}
