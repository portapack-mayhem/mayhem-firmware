# Stable Mayhem + Meteor — canonical Docker build (Windows PowerShell).
# Run from repo root:  .\scripts\stable-meteor-docker.ps1
# Optional: -Clean only removes build\ ; -NoBuild skips docker run (image only).
# If Docker bind-mount fails on Windows, use:  .\scripts\stable-meteor-docker.ps1 -UseWsl

param(
    [switch]$Clean,
    [switch]$NoBuild,
    # Run the same flow inside WSL when Windows Docker bind mounts fail (I/O or mkdir /host/c errors).
    [switch]$UseWsl
)

$ErrorActionPreference = "Stop"
$Root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $Root

Write-Host "== Submodules ==" -ForegroundColor Cyan
git submodule update --init --recursive

if ($UseWsl) {
    Write-Host "== Delegating to WSL + scripts/stable-meteor-docker.sh ==" -ForegroundColor Cyan
    $unixRoot = wsl -e wslpath -a $Root
    wsl -e bash -lc "cd '$unixRoot' && chmod +x scripts/stable-meteor-docker.sh scripts/share-out-collect.sh 2>/dev/null; ./scripts/stable-meteor-docker.sh"
    exit $LASTEXITCODE
}

if ($Clean -or -not $NoBuild) {
    Write-Host "== Clean build\ ==" -ForegroundColor Cyan
    if (Test-Path "$Root\build") { Remove-Item -Recurse -Force "$Root\build" }
    New-Item -ItemType Directory -Path "$Root\build" -Force | Out-Null
}

Write-Host "== Docker image ==" -ForegroundColor Cyan
docker build -f dockerfile-nogit -t portapack-dev $Root

if (-not $NoBuild) {
    $logDir = Join-Path $Root "share-out\logs"
    $artDir = Join-Path $Root "share-out\artifacts"
    New-Item -ItemType Directory -Force -Path $logDir, $artDir | Out-Null
    $ts = Get-Date -Format "yyyyMMddTHHmmssZ"
    $logFile = Join-Path $logDir "docker-make-$ts.log"
    Write-Host "== Full build (log: $logFile) ==" -ForegroundColor Cyan
    $jobs = if ($env:MAYHEM_MAKE_JOBS) { $env:MAYHEM_MAKE_JOBS } else { "1" }
    $flashMb = if ($env:MAYHEM_FLASH_MB_SIZE) { $env:MAYHEM_FLASH_MB_SIZE } else { "2" }
    $flashLimit = if ($env:MAYHEM_FLASH_MB_LIMIT) { $env:MAYHEM_FLASH_MB_LIMIT } else { "1" }
    docker run --rm -v "${Root}:/havoc" portapack-dev `
        -DFLASH_MB_SIZE=$flashMb -DFLASH_MB_LIMIT_SIZE=$flashLimit make -j$jobs 2>&1 | Tee-Object -FilePath $logFile
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host "== Artifacts in build\ ==" -ForegroundColor Cyan
    @(
        "$Root\build\firmware\portapack-mayhem-firmware.bin",
        "$Root\build\firmware\portapack-mayhem_OCI.ppfw.tar",
        "$Root\build\firmware\application\meteor_lrpt_rx.ppma"
    ) | ForEach-Object { if (Test-Path $_) { Get-Item $_ | Format-Table Name, Length -AutoSize } else { Write-Warning "Missing: $_" } }
    Write-Host "== share-out\artifacts ==" -ForegroundColor Cyan
    $bin = "$Root\build\firmware\portapack-mayhem-firmware.bin"
    $ppfw = "$Root\build\firmware\portapack-mayhem_OCI.ppfw.tar"
    $ppma = "$Root\build\firmware\application\meteor_lrpt_rx.ppma"
    foreach ($f in @($bin, $ppfw, $ppma)) {
        if (-not (Test-Path $f)) { Write-Error "share-out-collect: missing $f"; exit 1 }
        Copy-Item -Force $f $artDir
    }
    $sums = Join-Path $artDir "SHA256SUMS.txt"
    @($bin, $ppfw, $ppma) | ForEach-Object { Get-FileHash -Algorithm SHA256 $_ } | ForEach-Object { "$($_.Hash.ToLower())  $(Split-Path $_.Path -Leaf)" } | Set-Content -Encoding utf8 $sums
    $binLen = (Get-Item $bin).Length
    if ($binLen -ne 1048576) {
        Write-Warning "portapack-mayhem-firmware.bin is $binLen bytes (expected 1048576 for 1 MiB H4M)"
    }
    Write-Host "Copied firmware bundle to $artDir (see SHA256SUMS.txt)" -ForegroundColor Green
}

Write-Host "Done. 1 MiB H4M: scripts\H4M_METEOR_BUILD.txt | Flash: SD\FIRMWARE\ ppfw.tar ; recovery: flashing\mayhem_flasher.bat (DFU)." -ForegroundColor Green
