@echo off

echo *** Re-flash your device with PortaPack Mayhem firmware ***
echo.
echo Connect your HackRF One to a USB port on your computer.
echo.
echo If using a PortaPack, put the PortaPack in HackRF mode by selecting
echo the "HackRF" option from the main menu.
echo.

echo Please select your device:
echo.
echo   1. HackRF / PortaPack  (default)
echo   2. PortaRF
echo   3. HackRF Pro
echo.
set /p DEVICE_CHOICE="Enter your choice (1, 2 or 3): "

if "%DEVICE_CHOICE%"=="1" set FIRMWARE=firmware_hackrf.bin
if "%DEVICE_CHOICE%"=="2" set FIRMWARE=firmware_portarf.bin
if "%DEVICE_CHOICE%"=="3" set FIRMWARE=firmware_hpro.bin

if not defined FIRMWARE (
    echo.
    echo Invalid choice. Please run the script again and enter 1, 2, or 3.
    echo.
    pause
    exit /b
)

echo.
echo You selected: %FIRMWARE%
echo.

REM Check if the firmware file exists
if not exist %FIRMWARE% (
    echo The firmware file "%FIRMWARE%" does not exist.
    echo Please ensure that you have downloaded the latest release from:
    echo https://github.com/portapack-mayhem/mayhem-firmware/releases/
    echo.
    pause
    exit /b
)

pause

echo.
"utils/hackrf_spiflash.exe" -w %FIRMWARE%
echo.
echo If your device never boots after flashing, please refer to the won't boot article:
echo.
echo   click-to-open   : https://github.com/portapack-mayhem/mayhem-firmware/wiki/Won%%27t-boot
echo   copy-and-paste  : https://github.com/portapack-mayhem/mayhem-firmware/wiki/Won't-boot
echo.
pause
