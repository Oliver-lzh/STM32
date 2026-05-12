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

#include <common/typeutils.h>
#include <components/interfaces/IRadarSmartar.h>
#include <impl/PlatformInterruptDefinition.h>


// Smartar FPGA is wired to the RadarBaseboardMCU7 SPI wing pads
static const BoardRadarDefinition_t BoardRadarDefinitionSmartar[] = {
    [0] = {
        .devId     = 2,
        .dataIndex = 0,
    },
};

static IPinsSmartarDefinition_t BoardPinsSmartar[ARRAY_SIZE(BoardRadarDefinitionSmartar)];

static PlatformInterruptDefinition_t BoardIrqPinsSmartar[ARRAY_SIZE(BoardRadarDefinitionSmartar)][4] = {
    [0] = {
        // DIO5
        [0] = {
            // .pins is assigned in Board.c based on the detected shield connector
            .ul_attribute = PIO_DEFAULT,
            .ul_attr      = PIO_IT_RISE_EDGE,
        },
        // DIO4
        [1] = {
            // .pins is assigned in Board.c based on the detected shield connector
            .ul_attribute = PIO_DEFAULT,
            .ul_attr      = PIO_IT_RISE_EDGE,
        },
        // DIO3
        [2] = {
            // .pins is assigned in Board.c based on the detected shield connector
            .ul_attribute = PIO_DEFAULT,
            .ul_attr      = PIO_IT_RISE_EDGE,
        },
        // DIO2
        [3] = {
            // .pins is assigned in Board.c based on the detected shield connector
            .ul_attribute = PIO_DEFAULT,
            .ul_attr      = PIO_IT_RISE_EDGE,
        },
    },
};