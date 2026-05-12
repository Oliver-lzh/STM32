# RadarBaseboardMCU7 Firmware

This directory contains the RadarBaseboardMCU7 firmware.
In the release root directory you can find a tool to update the firmware.

## Compiling the Firmware

Open `RadarBaseboardMCU7.atsln` with Microchip (former Atmel) Studio and compile the code.
Alternatively, you can also call `make` from a command line.

## Precompiled Firmwares

Precompiled versions of the firmware can be found on following link under the folder
`Hatvan_FW`: https://sec-ishare.infineon.com/sites/Radar_SW_Releases/

## Flashing the Firmware

On Windows you can use the scripts `flash_debug.bat` and `flash_release.bat` located
in the `scripts` subfolder to flash a new firmware to the RadarBaseboardMCU7.
