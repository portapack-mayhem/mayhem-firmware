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
hackrf_update.exe hackrf_one_usb.bin
echo.
pause
