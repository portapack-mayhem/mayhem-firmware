@echo off

echo *** Run HackRF firmware in RAM via LPC DFU ***
echo.
echo This is used to "unbrick" your HackRF, if you are no longer able to use
echo HackRF tools to flash or operate your HackRF.
echo.
echo Connect your HackRF One to a USB port on your computer.
echo.
echo Hold down both the DFU and RESET buttons on the HackRF.
echo Then release the RESET button (closest to the edge).
echo Then release the DFU button.
echo.
pause

echo.
dfu-util-static.exe --device 1fc9:000c --download hackrf_one_usb.dfu --reset
echo.
pause
