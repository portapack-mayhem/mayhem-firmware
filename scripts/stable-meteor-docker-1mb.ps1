# 1 MiB SPI Mayhem + Meteor — delegates to WSL (recommended) or native Docker.
param([switch]$UseWsl = $true)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$env:MAYHEM_FLASH_MB_SIZE = "2"
$env:MAYHEM_FLASH_MB_LIMIT = "1"

if ($UseWsl) {
    $unixRoot = wsl -e wslpath -a $Root
    # wsl-build-share-out: rsync to ext4, submodule hackrf on ext4, sync share-out/ back to Windows.
    wsl -e bash -lc "cd '$unixRoot' && sed -i 's/\r$//' scripts/*.sh && chmod +x scripts/*.sh && MAYHEM_CLEAN_BUILD=\${MAYHEM_CLEAN_BUILD:-0} ./scripts/wsl-build-share-out.sh"
    exit $LASTEXITCODE
}

& (Join-Path $PSScriptRoot "stable-meteor-docker.ps1")
