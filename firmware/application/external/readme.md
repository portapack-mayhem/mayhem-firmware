# External apps developer guide

External apps are built into the same ELF as the firmware, but linked into their own 32k regions and exported as separate `.ppma` images that are loaded from the SD card. Depending on the flash size of the build, some of them are instead compiled **into** the firmware ("internal").

## Flash tiers (builds)

The root `CMakeLists.txt` picks the build's `FLASH_TIER` from the flash size and uses the matching linker script:

| `FLASH_TIER` | Flash | Typical device | Linker script | Internal apps (`tier.txt` value) |
|---|---|---|---|---|
| 0 | <= 1 MB | everything external | `external_tier0.ld` | none |
| 1 | <= 2 MB | PortaRF | `external_tier1.ld` | 1 |
| 2 | > 2 MB | HackRF Pro | `external_tier2.ld` | 1 and 2 |

## Marking an app: `tier.txt`

Every app folder has a `tier.txt` containing a single number: **the first build tier in which the app is compiled into the firmware (internal)**. The tier 0 build (<= 1 MB) has no room for internal apps, so every app is external there. A missing file means `0`.

| `tier.txt` | Behaviour | External in builds | Internal in builds | Use for |
|---|---|---|---|---|
| `-1` | not compiled at all | - | - | retired apps whose code is kept in the repo |
| `0` | never internal | 0, 1, 2 | - | joke, unnecessary or harmful apps (like jammer) |
| `1` | internal in PortaRF (2 MB) and HackRF Pro | 0 | 1, 2 | the most important / most interesting apps |
| `2` | internal in HackRF Pro only | 0, 1 | 2 | other useful apps |

A bigger number means "stays external longer" (except `0`, which is external everywhere). `external_tierT.ld` lists the apps whose `tier.txt` is `0` or greater than `T`.

## Adding a new external app

1. Create `external/<app>/` with a `main.cpp` and your sources. `main.cpp` defines `initialize_app` and the `application_information` struct in the section `.external_app.app_<name>.application_information`. `<name>` is the app's *section name*: usually the folder name, but not always (folder `foxhunt` -> `foxhunt_rx`). It must be unique, and there must be exactly one such section in the folder.
2. Add the sources to `EXTCPPSRC` in `external.cmake` (`main.cpp` is required, and every other file that needs to be compiled too) and the section name to `EXTAPPLIST` in the same file.
3. Create `external/<app>/tier.txt` (see above).
4. Run `python3 tools/update_external_tier_ld.py` (see below). Never edit the `external_tier*.ld` files by hand.
5. If `tier.txt` is `1` or `2`, add the header include and the menu entry to `ui_navigation.cpp` (see below). Otherwise skip this step.
6. Handle the baseband image (see below).

## `tools/update_external_tier_ld.py`

Regenerates `external_tier0.ld`, `external_tier1.ld` and `external_tier2.ld` from the `tier.txt` files and verifies that the rest of the tree is consistent. It only ever writes those three files.

- Regions are 32k, contiguous, in 0x10000 steps from `0xADB10000`. Apps keep their current order in the files and new apps are appended alphabetically. Addresses shift when an app is added or removed; this is expected, because the exporter patches the addresses after the build.
- Checks (any failure exits with status 1 and prints `ERROR: ...`):
  - **`ui_navigation.cpp`**: an app with `tier.txt` = `N` (>= 1) must be `#include`d from `external/<dir>/...` under `#if FLASH_TIER >= N`. An app with `0` must not be included at all.
  - **`external.cmake`**: the app's `main.cpp` must be in `EXTCPPSRC` and its section name in `EXTAPPLIST` (commented-out entries don't count).
  - Apps with `-1` are left out of the `.ld` files and skip both checks.
- `--check` writes nothing and exits with 1 if the `.ld` files are out of date.

The script runs automatically on every CMake configure, **before** the selected `.ld` file is copied to the build directory as `external.ld`, and a failure aborts the build. The `tier.txt` files and `ui_navigation.cpp` are configure dependencies, so editing them re-runs it. Commit the regenerated `.ld` files together with your change. Changing an app's sources or section name does not retrigger CMake by itself, so re-run CMake (or the script) manually in that case.

## `ui_navigation.cpp` (include and menu entry)

Apps with `tier.txt` = `N` >= 1 are compiled into the firmware from build tier `N` up. Internal apps are started from the normal menus, so they need two things in [ui_navigation.cpp](../../ui_navigation.cpp), both guarded with `#if FLASH_TIER >= N` (use the app's own `N`):

1. **Header include**, in the guarded block near the top of the file:

   ```cpp
   #if FLASH_TIER >= 1
   #include "external/adsbrx/ui_adsb_rx.hpp"
   #endif
   ```

2. **Menu entry**, in the `appList` array, in the section of the menu where it should show up (`RX`, `TX`, `TRX`, `UTILITIES`, ...), wrapped in the same kind of guard:

   ```cpp
   #if FLASH_TIER >= 1
       {"adsbrx", "ADS-B", RX, Color::green(), &bitmap_icon_adsb, new ViewFactory<ui::external_app::adsbrx::ADSBRxView>()},
   #endif
   ```

   The fields are: call name, displayed name, menu location, color, icon, and a `ViewFactory` of the app's main view class from namespace `ui::external_app::<name>`.

If you put the include or the entry under the wrong `N`, the build stops with an error from `tools/update_external_tier_ld.py`. Only the include is checked, so keep the menu entry's guard identical to it. Apps with `0` or `-1` must have neither: they are listed by the external app loader instead, in every build where they are external.

## Baseband images

Open `firmware/baseband/CMakeLists.txt` and read the instructions there. It has tiered sections ("EXTERNAL ON TIER 0", "TIER 0, 1", "TIER 2, so external on all tiers"). Put the baseband image in the section that matches the app's `tier.txt`, so it is external exactly when the app is: `1` -> "external on tier 0", `2` -> "external on tier 0, 1", `0` -> "external on all tiers". If several apps share one baseband image, use the section of the app that stays external the *longest*, because the image must be external in every build where any of its users is. Example: used by a `1` app and a `0` app, so it goes in the "external on all tiers" section.

## Starting the baseband from an external app

Don't use `baseband::run_prepared_image(portapack::memory::map::m4_code.base())`. It only works when the baseband is bundled with the external app. Use for example `baseband::run_image(portapack::spi_flash::image_tag_tpms);` instead. It falls back to loading the image from the external app on builds that don't have it in the firmware, so it works in every tier, even if the app's marking changes later.
