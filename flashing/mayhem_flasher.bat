@echo off

echo =========================================================
echo   PortaPack Mayhem - Device Flasher
echo =========================================================
echo.
echo Connect your device to a USB port on your computer.
echo.

REM ── Step 1: Select device ─────────────────────────────────
echo What is your device?
echo.
echo   1. HackRF One / PortaPack
echo   2. PortaRF
echo   3. HackRF Pro / PortaPack
echo.
set /p DEVICE_CHOICE="Enter your choice (1, 2 or 3): "

if "%DEVICE_CHOICE%"=="1" set DEVICE_NAME=HackRF One / PortaPack
if "%DEVICE_CHOICE%"=="2" set DEVICE_NAME=PortaRF
if "%DEVICE_CHOICE%"=="3" set DEVICE_NAME=HackRF Pro / PortaPack

if not defined DEVICE_NAME (
    echo.
    echo Invalid choice. Please run the script again and enter 1, 2, or 3.
    echo.
    pause
    exit /b
)

echo.
echo Device: %DEVICE_NAME%
echo.

REM ── Step 2: Select action ──────────────────────────────────
echo What would you like to do?
echo.
echo   1. Flash Mayhem firmware
echo   2. Flash DFU then Mayhem  (DFU unbrick followed by Mayhem flash in one go)
echo   3. Flash via DFU  (unbrick - run HackRF firmware from RAM)
echo   4. Flash factory HackRF firmware  (HackRF only - removes PortaPack support)
echo.
set /p ACTION_CHOICE="Enter your choice (1, 2, 3 or 4): "

if "%ACTION_CHOICE%"=="1" goto :flash_mayhem
if "%ACTION_CHOICE%"=="2" goto :flash_dfu_then_mayhem
if "%ACTION_CHOICE%"=="3" goto :flash_dfu
if "%ACTION_CHOICE%"=="4" goto :flash_factory

echo.
echo Invalid choice. Please run the script again and enter 1, 2, 3, or 4.
echo.
pause
exit /b


REM ── Action 1: Flash Mayhem ─────────────────────────────────
:flash_mayhem
echo.
if "%DEVICE_CHOICE%"=="1" set FIRMWARE=firmware\firmware_hackrf.bin
if "%DEVICE_CHOICE%"=="2" set FIRMWARE=firmware\firmware_portarf.bin
if "%DEVICE_CHOICE%"=="3" set FIRMWARE=firmware\firmware_hpro.bin

if "%DEVICE_CHOICE%"=="1" (
    echo If your device has a PortaPack attached, switch it to HackRF mode first by
    echo selecting the "HackRF" option from the main menu.
    echo.
)
echo Firmware: %FIRMWARE%
echo.

if not exist %FIRMWARE% (
    echo ERROR: The firmware file "%FIRMWARE%" was not found.
    echo Please ensure you have downloaded the latest release from:
    echo   https://release.hackrf.app/
    echo.
    pause
    exit /b
)

echo.
"utils/hackrf_spiflash.exe" -R -w %FIRMWARE%
echo.
echo If your device does not boot after flashing, see the troubleshooting wiki:
echo   https://github.com/portapack-mayhem/mayhem-firmware/wiki/Wont-boot
echo.
pause
exit /b


REM ── Action 2: DFU unbrick ──────────────────────────────────
:flash_dfu
echo.
echo *** Load HackRF firmware into RAM via LPC DFU ***
echo.
echo Use this to unbrick your device if you can no longer use HackRF tools
echo to flash or communicate with it.
echo.
echo Before pressing any key, put your device into DFU mode:
echo   1. Hold down both the DFU and RESET buttons.
echo   2. Release the RESET button first (the one closest to the edge).
echo   3. Then release the DFU button.
echo.

if "%DEVICE_CHOICE%"=="3" (
    set DFU_FILE=firmware\hackrf_hpro_usb.dfu
) else (
    set DFU_FILE=firmware\hackrf_usb.dfu
)

if not exist "%DFU_FILE%" (
    echo ERROR: "%DFU_FILE%" was not found.
    echo Please ensure you have downloaded the latest release from:
    echo   https://release.hackrf.app/
    echo.
    pause
    exit /b
)

echo.
"utils/dfu-util-static.exe" --device 1fc9:000c --download "%DFU_FILE%"
echo.
pause
exit /b


REM ── Action 3: DFU then flash Mayhem ─────────────────────────
:flash_dfu_then_mayhem
echo.
echo *** DFU unbrick followed by Mayhem firmware flash ***
echo.
echo This will first load HackRF firmware into RAM via DFU, then flash Mayhem.
echo.
echo Before pressing any key, put your device into DFU mode:
echo   1. Hold down both the DFU and RESET buttons.
echo   2. Release the RESET button first (the one closest to the edge).
echo   3. Then release the DFU button.
echo.

if "%DEVICE_CHOICE%"=="3" (
    set DFU_FILE=firmware\hackrf_hpro_usb.dfu
) else (
    set DFU_FILE=firmware\hackrf_usb.dfu
)

if not exist "%DFU_FILE%" (
    echo ERROR: "%DFU_FILE%" was not found.
    echo Please ensure you have downloaded the latest release from:
    echo   https://release.hackrf.app/
    echo.
    pause
    exit /b
)

if "%DEVICE_CHOICE%"=="1" set FIRMWARE=firmware\firmware_hackrf.bin
if "%DEVICE_CHOICE%"=="2" set FIRMWARE=firmware\firmware_portarf.bin
if "%DEVICE_CHOICE%"=="3" set FIRMWARE=firmware\firmware_hpro.bin

if not exist "%FIRMWARE%" (
    echo ERROR: The firmware file "%FIRMWARE%" was not found.
    echo Please ensure you have downloaded the latest release from:
    echo   https://release.hackrf.app/
    echo.
    pause
    exit /b
)

echo Step 1 of 2: Loading HackRF firmware via DFU...
echo.
"utils/dfu-util-static.exe" --device 1fc9:000c --download "%DFU_FILE%"

echo.
echo DFU complete. Waiting 5 seconds for device to re-enumerate...
timeout /t 5 /nobreak >nul

echo.
echo Step 2 of 2: Flashing Mayhem firmware (%FIRMWARE%)...
echo.
"utils/hackrf_spiflash.exe" -R -w "%FIRMWARE%"
echo.
echo If your device does not boot after flashing, see the troubleshooting wiki:
echo   https://github.com/portapack-mayhem/mayhem-firmware/wiki/Wont-boot
echo.
pause
exit /b


REM ── Action 4: Flash factory HackRF firmware ────────────────
:flash_factory
echo.
echo *** Restore factory HackRF firmware ***
echo.
echo This will remove Mayhem and restore the original HackRF firmware.
echo PortaPack functionality will no longer be available after this.
echo.

if "%DEVICE_CHOICE%"=="3" (
    set FACTORY_BIN=firmware\hackrf_hpro_usb.bin
) else (
    set FACTORY_BIN=firmware\hackrf_usb.bin
)

if not exist "%FACTORY_BIN%" (
    echo ERROR: "%FACTORY_BIN%" was not found.
    echo Please ensure you have downloaded the latest release from:
    echo   https://release.hackrf.app/
    echo.
    pause
    exit /b
)

echo.
"utils/hackrf_spiflash.exe" -R -w "%FACTORY_BIN%"
echo.
pause
exit /b
