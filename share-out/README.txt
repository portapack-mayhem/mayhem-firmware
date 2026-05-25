This directory is populated by the stable Meteor Docker workflow (see scripts/).

  logs/        Full docker make output (tee), one file per run (UTC timestamp in filename).
  artifacts/   portapack-mayhem-firmware.bin, portapack-mayhem_OCI.ppfw.tar,
               meteor-capture.ppma, meteor-decode.ppmp, meteor-view.ppmp,
               SHA256SUMS.txt from the same build.

H4M / 1 MiB SPI (W25Q80-class) — default for scripts/stable-meteor-docker-1mb.sh:
  CMake: -DFLASH_MB_SIZE=2 -DFLASH_MB_LIMIT_SIZE=1 (SPI map vs packed 1 MiB .bin — do not set both to 1)
  portapack-mayhem-firmware.bin must be 1048576 bytes before flashing.
  Canonical flash path: PC USB + hackrf_spiflash (see scripts/flash-h4m-share-out.ps1). SD Flash Utility is experimental until read-back is proven.
  Flash one OCI tar plus all three Meteor APPS from the same build (see scripts/H4M_METEOR_BUILD.txt).

If WSL/Docker fails with "Input/output error" under /mnt/c, resume on Linux fs:
  (see scripts/STABLE_BUILD_NOTES.txt — sync to ~/mayhem-firmware-1mb and docker-resume-share-out.sh)

Do not mix artifacts from different runs on the SD card.
