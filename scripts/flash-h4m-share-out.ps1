# Flash PortaPack H4M (HackRF One + H4M) from share-out/artifacts via USB — 1 MiB SPI only.
# Canonical host path for H4M: hackrf_spiflash (this script). SD Flash Utility is experimental until read-back is proven.
# Requires flashing\utils\hackrf_spiflash.exe (see flashing\README.txt).
# Device must be in HackRF mode (not PortaPack menu) for SPI flash from the host.
#
# Usage (repo root):
#   .\scripts\flash-h4m-share-out.ps1
# Optional: -BinPath path\to\custom.bin
# Optional: -AllowNonStandardSize (not recommended)

param(
    [string]$BinPath = "",
    [switch]$AllowNonStandardSize
)

$ErrorActionPreference = "Stop"
$ExpectedSize = 1048576
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$DefaultBin = Join-Path $Root "share-out\artifacts\portapack-mayhem-firmware.bin"
$SpiFlash = Join-Path $Root "flashing\utils\hackrf_spiflash.exe"

if ($BinPath) {
    $Bin = (Resolve-Path $BinPath).Path
} elseif (Test-Path $DefaultBin) {
    $Bin = $DefaultBin
} else {
    Write-Error 'No firmware .bin found. Run WSL: ./scripts/stable-meteor-docker-1mb.sh (or .\scripts\stable-meteor-docker.ps1 -UseWsl) first, or pass -BinPath.'
}

if (-not (Test-Path $SpiFlash)) {
    Write-Error "Missing $SpiFlash - get flashing\utils from https://release.hackrf.app/ (see flashing\README.txt)."
}

$pyCheck = Join-Path $Root "scripts\check_spi_no_pmlr_chunk.py"
if (Test-Path $pyCheck) {
    python $pyCheck $Bin
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
} elseif ([System.Text.Encoding]::ASCII.GetString([System.IO.File]::ReadAllBytes($Bin)).Contains('PMLR')) {
    Write-Error "This .bin contains a PMLR SPI chunk. Rebuild: MAYHEM_CLEAN_BUILD=1 ./scripts/wsl-build-share-out.sh"
}

$len = (Get-Item $Bin).Length
if ($len -ne $ExpectedSize) {
    $msg = "Firmware must be exactly $ExpectedSize bytes (1 MiB SPI). Got $len bytes. Build with scripts/stable-meteor-docker-1mb.sh. A 2 MiB image will brick or blank a 1 MiB SPI part."
    if ($AllowNonStandardSize) {
        Write-Warning $msg
    } else {
        Write-Error $msg
    }
}

Write-Host 'H4M / HackRF One SPI flash (1 MiB) — canonical USB path' -ForegroundColor Cyan
Write-Host "  Image: $Bin - $len bytes"
Write-Host "  Tool:  $SpiFlash"
Write-Host ''
Write-Host 'Put the PortaPack in HackRF mode, connect USB, then confirm.' -ForegroundColor Yellow
$ok = Read-Host 'Flash now? (y/N)'
if ($ok -ne 'y' -and $ok -ne 'Y') { exit 0 }

# Release-style write + reset (1 MiB image must pass compatibility check).
# -i skips the check — only use manually if you know why.

& $SpiFlash -R -w $Bin
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ''
Write-Host 'SPI flash OK (image is on SPI flash).' -ForegroundColor Green
Write-Host ''
Write-Host 'IMPORTANT — boot PortaPack Mayhem (not the same as SPI write success):' -ForegroundColor Yellow
Write-Host '  1. Unplug USB (recommended) or leave it unplugged after the tool exits.'
Write-Host '  2. Press the RESET button on the HackRF One (short press).'
Write-Host '  3. The PortaPack screen should show Mayhem. TX/RX LEDs may blink during USB'
Write-Host '     HackRF mode — that is normal until you reset; it is not a failed flash.'
Write-Host ''
Write-Host 'If the screen stays blank after RESET:' -ForegroundColor Yellow
Write-Host '  Recover: DFU + flashing\mayhem_flasher.bat option 2 (official 1 MiB), then retry.'
Write-Host '  See scripts\BOOT_FAILURE_H4M.md'
Write-Host ''
Write-Host 'Meteor LRPT (same build) -> SD card APPS\:' -ForegroundColor Cyan
Write-Host '  meteor-capture.ppma'
Write-Host '  meteor-decode.ppmp'
Write-Host '  meteor-view.ppmp'
Write-Host '  (or flash portapack-mayhem_OCI.ppfw.tar via Flash Utility — experimental)'
Write-Host 'See scripts\H4M_METEOR_BUILD.txt'
Write-Host ''
Write-Host 'After boot (SD card):' -ForegroundColor Cyan
Write-Host '  Keep SETTINGS / SD Card / enable high speed IO OFF unless you tested it.'
Write-Host '  If boot hangs only with SD inserted: SETTINGS / P.Memory Mgmt / Load mem defaults, reboot.'
Write-Host '  See https://github.com/portapack-mayhem/mayhem-firmware/wiki/Won''t-boot'
