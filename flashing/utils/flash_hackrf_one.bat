@echo off

echo *** Re-flash the HackRF with HackRF firmware ***
echo.
echo Connect your HackRF One to a USB port on your computer.
echo.
echo If using a PortaPack, put the PortaPack in HackRF mode by selecting
echo the "HackRF" option from the main menu.
echo.
pause

echo.
"utils/hackrf_spiflash.exe" -w "utils/hackrf_usb.bin"
echo.
pause
