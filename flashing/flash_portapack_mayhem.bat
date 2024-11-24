@echo off

echo *** Re-flash the HackRF with PortaPack firmware ***
echo.
echo Connect your HackRF One to a USB port on your computer.
echo.
echo If using a PortaPack, put the PortaPack in HackRF mode by selecting
echo the "HackRF" option from the main menu.
echo.
pause

echo.

REM Check if the firmware file exists
if not exist portapack-mayhem-firmware.bin (
    echo The firmware file "portapack-mayhem-firmware.bin" does not exist.
    echo Please ensure that you have downloaded the latest release from:
    echo https://github.com/portapack-mayhem/mayhem-firmware/releases/
    echo.
    pause
    exit /b
)

"utils/hackrf_update.exe" portapack-mayhem-firmware.bin
echo.
echo If your device never boot after flashing, please refer to https://github.com/portapack-mayhem/mayhem-firmware/wiki/Won't-boot
echo.
pause
