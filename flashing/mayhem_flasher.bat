@echo off
setlocal EnableDelayedExpansion

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
call :set_firmware
if "%DEVICE_CHOICE%"=="1" (
    echo If your device has a PortaPack attached, switch it to HackRF mode first by
    echo selecting the "HackRF" option from the main menu.
    echo.
)
echo Firmware: %FIRMWARE%
echo.
call :check_file_exists "%FIRMWARE%" "firmware file" || (pause & exit /b)
call :do_flash
if "!FLASH_OK!"=="1" call :print_success
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
call :set_dfu_file
call :check_file_exists "%DFU_FILE%" "DFU file" || (pause & exit /b)
echo.
call :do_dfu
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
call :set_dfu_file
call :check_file_exists "%DFU_FILE%" "DFU file" || (pause & exit /b)
call :set_firmware
call :check_file_exists "%FIRMWARE%" "firmware file" || (pause & exit /b)

echo Step 1 of 2: Loading HackRF firmware via DFU...
echo.
call :do_dfu

echo.
echo DFU complete. Waiting 5 seconds for device to reconnect...
timeout /t 5 /nobreak >nul

echo.
echo Step 2 of 2: Flashing Mayhem firmware (%FIRMWARE%)...
echo.
call :do_flash
if "!FLASH_OK!"=="1" call :print_success
pause
exit /b


REM ── Serial fallback: boot device into HackRF mode via serial ──
:serial_fallback
set SERIAL_OK=0
set COM_PORT=
echo.

REM Enumerate available COM ports into a temp file
powershell -NoProfile -Command "[System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object | ForEach-Object { $_ }" > "%TEMP%\mayhem_comports.txt" 2>nul

REM Count and index ports
set PORT_COUNT=0
for /f "usebackq tokens=*" %%P in ("%TEMP%\mayhem_comports.txt") do (
    set /a PORT_COUNT+=1
    set PORT_!PORT_COUNT!=%%P
)

if !PORT_COUNT! equ 0 (
    goto :eof
)

if !PORT_COUNT! equ 1 (
    set COM_PORT=!PORT_1!
) else (
    echo Multiple COM ports detected. Select the one your device is on:
    echo.
    for /l %%I in (1,1,!PORT_COUNT!) do (
        echo   %%I. !PORT_%%I!
    )
    echo.
    set /p PORT_CHOICE="Enter your choice (1-!PORT_COUNT!): "
    set COM_PORT=
    for /l %%I in (1,1,!PORT_COUNT!) do (
        if "!PORT_CHOICE!"=="%%I" set COM_PORT=!PORT_%%I!
    )
    if not defined COM_PORT (
        echo Invalid selection.
        goto :eof
    )
)

echo Connecting to !COM_PORT!...
powershell -NoProfile -Command "$p = New-Object System.IO.Ports.SerialPort('!COM_PORT!', 115200, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One); try { $p.Open(); $p.Write('hackrf' + [char]13 + [char]10); Start-Sleep -Milliseconds 500 } catch { Write-Host ('Serial error: ' + $_.Exception.Message); exit 1 }; try { $p.Close() } catch {}; exit 0"
if %ERRORLEVEL% equ 0 (
    set SERIAL_OK=1
    echo Switching to HackRF mode, waiting for device...
    timeout /t 7 /nobreak >nul
) else (
    echo Could not open !COM_PORT!. Make sure the port is not in use.
)
goto :eof


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

call :check_file_exists "%FACTORY_BIN%" "factory firmware" || (pause & exit /b)
echo.
"utils/hackrf_spiflash.exe" -R -w "%FACTORY_BIN%"
echo.
pause
exit /b


REM ── Helper: Set FIRMWARE based on DEVICE_CHOICE ─────────────
:set_firmware
if "%DEVICE_CHOICE%"=="1" set FIRMWARE=firmware\firmware_hackrf.bin
if "%DEVICE_CHOICE%"=="2" set FIRMWARE=firmware\firmware_portarf.bin
if "%DEVICE_CHOICE%"=="3" set FIRMWARE=firmware\firmware_hpro.bin
goto :eof


REM ── Helper: Set DFU_FILE based on DEVICE_CHOICE ─────────────
:set_dfu_file
if "%DEVICE_CHOICE%"=="3" (
    set DFU_FILE=firmware\hackrf_hpro_usb.dfu
) else (
    set DFU_FILE=firmware\hackrf_usb.dfu
)
goto :eof


REM ── Helper: Check a required file exists ─────────────────────
REM   %1 = quoted file path   %2 = human-readable label
:check_file_exists
if not exist %1 (
    echo ERROR: The %~2 file %1 was not found.
    echo Please ensure you have downloaded the latest release from:
    echo   https://release.hackrf.app/
    echo.
    exit /b 1
)
exit /b 0


REM ── Helper: Flash FIRMWARE with serial fallback ───────────────
:do_flash
set FLASH_OK=0
"utils/hackrf_spiflash.exe" -R -w "%FIRMWARE%"
if %ERRORLEVEL% equ 0 (
    set FLASH_OK=1
    goto :eof
)
call :serial_fallback
if "!SERIAL_OK!"=="1" (
    echo.
    echo Device should now be in HackRF mode. Retrying flash...
    echo.
    "utils/hackrf_spiflash.exe" -R -w "%FIRMWARE%"
    if !ERRORLEVEL! equ 0 (
        set FLASH_OK=1
    ) else (
        echo.
        call :print_error
    )
) else (
    echo.
    call :print_error
)
goto :eof


REM ── Helper: Run DFU flash ────────────────────────────────────
:do_dfu
"utils/dfu-util-static.exe" --device 1fc9:000c --download "%DFU_FILE%"
goto :eof


REM ── Helper: Print error banner ──────────────────────────────
:print_error
echo.
echo  $$$$$$\                                
echo $$  __$$\                               
echo $$ /  $$ ^| $$$$$$\   $$$$$$\   $$$$$$$\ 
echo $$ ^|  $$ ^|$$  __$$\ $$  __$$\ $$  _____^|
echo $$ ^|  $$ ^|$$ /  $$ ^|$$ /  $$ ^|\$$$$$$\  
echo $$ ^|  $$ ^|$$ ^|  $$ ^|$$ ^|  $$ ^| \____$$\ 
echo  $$$$$$  ^|\$$$$$$  ^|$$$$$$$  ^|$$$$$$$  ^|
echo  \______/  \______/ $$  ____/ \_______/ 
echo                     $$ ^|                
echo                     $$ ^|                
echo                     \__^|                
echo.
echo   ERROR: Device not found. Make sure your device is connected,
echo   powered on, and in HackRF mode, then try again.
echo.
goto :eof


REM ── Helper: Print success banner ─────────────────────────────
:print_success
echo.
echo   ______   __    __   ______    ______   ________   ______    ______
echo  /      \ ^|  \  ^|  \ /      \  /      \ ^|        \ /      \  /      \
echo ^|  $$$$$$\^| $$  ^| $$^|  $$$$$$\^|  $$$$$$\^| $$$$$$$$^|  $$$$$$\^|  $$$$$$\
echo ^| $$___\$$^| $$  ^| $$^| $$   \$$^| $$   \$$^| $$__    ^| $$___\$$^| $$___\$$
echo  \$$    \ ^| $$  ^| $$^| $$      ^| $$      ^| $$  \    \$$    \  \$$    \
echo  _\$$$$$$\^| $$  ^| $$^| $$   __ ^| $$   __ ^| $$$$$    _\$$$$$$\ _\$$$$$$\
echo ^|  \__^| $$^| $$__/ $$^| $$__/  \^| $$__/  \^| $$_____ ^|  \__^| $$^|  \__^| $$
echo  \$$    $$ \$$    $$ \$$    $$ \$$    $$^| $$     \ \$$    $$ \$$    $$
echo   \$$$$$$   \$$$$$$   \$$$$$$   \$$$$$$  \$$$$$$$$  \$$$$$$   \$$$$$$
echo.
echo   Firmware flashed successfully. Your device is rebooting...
echo.
echo If your device does not boot after flashing, see the troubleshooting wiki:
echo   https://github.com/portapack-mayhem/mayhem-firmware/wiki/Wont-boot
echo.
goto :eof
