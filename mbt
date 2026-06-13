#!/usr/bin/env bash

set -euo pipefail

SCRIPT_PATH="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
DEFAULT_TOOLCHAIN_URL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/RC2.1/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2?revision=6e63531f-8cb1-40b9-bbfc-8a57cdfc01b4&la=en&hash=F761343D43A0587E8AC0925B723C04DBFB848339"

mbt_log() {
    printf "mbt: %s\n" "$*"
}

mbt_error() {
    printf "mbt: error: %s\n" "$*" >&2
}

mbt_die() {
    mbt_error "$*"
    exit 1
}

mbt_command_exists() {
    command -v "$1" >/dev/null 2>&1
}

mbt_default_jobs() {
    local jobs

    jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || true)"
    if [[ -z "$jobs" ]] && mbt_command_exists nproc; then
        jobs="$(nproc)"
    fi
    if [[ -z "$jobs" ]]; then
        jobs=1
    fi

    printf "%s" "$jobs"
}

mbt_detect_distro() {
    if mbt_command_exists apt-get; then
        printf "debian"
        return 0
    fi
    if mbt_command_exists pacman; then
        printf "arch"
        return 0
    fi
    printf "unknown"
}

mbt_run_privileged() {
    if [[ "${EUID:-$(id -u)}" -eq 0 ]]; then
        "$@"
        return
    fi

    if ! mbt_command_exists sudo; then
        mbt_die "sudo is required for dependency installation. Re-run as root or install sudo."
    fi

    sudo "$@"
}

mbt_install_deps() {
    local distro
    distro="$(mbt_detect_distro)"

    case "$distro" in
    debian)
        mbt_log "detected Debian-like environment; installing required packages."
        mbt_run_privileged apt-get update
        mbt_run_privileged apt-get install -y --no-install-recommends \
            git tar wget dfu-util cmake make python3 bzip2 lz4 curl hackrf \
            python3-setuptools python3-yaml binutils python3-setuptools ccache ninja-build
        ;;
    arch)
        mbt_log "detected Arch-like environment; installing required packages."
        mbt_run_privileged pacman -Sy --needed --noconfirm \
            git tar wget dfu-util cmake make python3 bzip2 lz4 curl hackrf \
            python-distutils-extra python-setuptools python-pip python-yaml \
            ninja ccache binutils
        ;;
    *)
        mbt_die "unsupported distribution. Install dependencies manually (see wiki on github), or you can send PR to support for your distro. Detected distro: $distro"
        ;;
    esac
}

mbt_sync_submodules() {
    local jobs
    jobs="$(mbt_default_jobs)"
    mbt_log "syncing submodules."
    git -C "$SCRIPT_PATH" submodule update --init --recursive --jobs "$((jobs * 2))"
}

mbt_enable_toolchain_env() {
    export MBT_ROOT="$SCRIPT_PATH"
    export MBT_TOOLCHAIN_DIR="${MBT_TOOLCHAIN_DIR:-$SCRIPT_PATH/armbin}"
    export MBT_TOOLCHAIN_URL="${MBT_TOOLCHAIN_URL:-$DEFAULT_TOOLCHAIN_URL}"
    export MBT_EXPECTED_GCC_VERSION="${MBT_EXPECTED_GCC_VERSION:-9.2.1}"

    if [[ -z "${MBT_FORCE_TOOLCHAIN+x}" ]]; then
        MBT_FORCE_TOOLCHAIN="${FORCE_TOOLCHAIN:-0}"
    fi
    export MBT_FORCE_TOOLCHAIN

    if [[ -z "${MBT_NO_TOOLCHAIN_DOWNLOAD+x}" ]]; then
        MBT_NO_TOOLCHAIN_DOWNLOAD="${NO_TOOLCHAIN_DOWNLOAD:-0}"
    fi
    export MBT_NO_TOOLCHAIN_DOWNLOAD
    export MBT_VERBOSE="${MBT_VERBOSE:-}"

    # shellcheck source=/dev/null
    . "$SCRIPT_PATH/scripts/toolchain/mbtenv.sh"
}

mbt_use_existing_toolchain_env() {
    local repo_toolchain_bin="$SCRIPT_PATH/armbin/bin"

    if [[ ! -d "$repo_toolchain_bin" ]]; then
        return 0
    fi

    case ":$PATH:" in
    *":$repo_toolchain_bin:"*)
        return 0
        ;;
    esac

    export PATH="$repo_toolchain_bin:$PATH"
}

mbt_normalize_device() {
    local raw
    raw="$(printf "%s" "$1" | tr '[:upper:]' '[:lower:]')"

    case "$raw" in
    1|one|hackrf|hackrf-one)
        printf "hackrf-one"
        ;;
    2|pro|hackrf-pro|praline)
        printf "hackrf-pro"
        ;;
    3|portarf|porta-rf|porta_rf)
        printf "portarf"
        ;;
    *)
        return 1
        ;;
    esac
}

mbt_prompt_device() {
    local choice normalized

    cat >&2 <<'EOF'
Select device type:
  1) HackRF One
  2) HackRF Pro (PRALINE)
  3) PortaRF
EOF

    while true; do
        printf "Device [1-3]: " >&2
        if ! IFS= read -r choice; then
            return 1
        fi

        normalized="$(mbt_normalize_device "$choice" 2>/dev/null || true)"
        if [[ -n "$normalized" ]]; then
            printf "%s" "$normalized"
            return 0
        fi
        mbt_error "invalid device selection: $choice"
    done
}

mbt_print_help() {
    cat <<'EOF'
Usage:
  ./mbt                          One-step build flow (installs deps, syncs submodules, downloads toolchain, asks for device, builds)
  ./mbt build [options]          Developer build flow
  ./mbt deps                     Install distro dependencies only
  ./mbt toolchain [--force-toolchain]
                                 Download/setup ARM toolchain only
  ./mbt doctor                   Validate host setup and report gaps
  ./mbt shell [options]          Open a shell with toolchain exported for this session

Build options:
  -d, --device <name>            Device: hackrf-one | hackrf-pro | portarf
  --ccache / --no-ccache         Toggle USE_CCACHE (default: on)
  --with-deps / --no-deps        Install Linux dependencies before build
  --with-submodule-sync          Update git submodules before build (default)
  --no-submodule-sync            Skip submodule sync
  --with-toolchain               Ensure toolchain exists and matches expected version (default)
  --no-toolchain                 Skip toolchain checks/downloads and use existing PATH (prepends ./armbin/bin if present)
  --force-toolchain              Re-download and reinstall toolchain
  -B, --build-dir <path>         Build directory (default: ./build)
  -G, --generator <name>         CMake generator (default: Ninja)
  -j, --jobs <n>                 Build parallelism (default: host CPU count)
  --target <name>                Build a specific target
    --clean                        Remove build directory, run libopencm3 clean, and exit
  --cmake-arg <arg>              Extra CMake configure arg (repeatable)
  --build-arg <arg>              Extra cmake --build arg (repeatable)
  -y, --non-interactive          Fail instead of prompting when required input is missing
  -h, --help                     Show this help

Examples:
  ./mbt
  ./mbt build --device hackrf-one
  ./mbt build --device hackrf-pro --no-submodule-sync
  ./mbt build --device portarf --no-toolchain --no-deps
  ./mbt build --device hackrf-one --cmake-arg=-DVERSION_STRING=my-build
EOF
}

mbt_doctor() {
    local distro missing=0 toolchain_cc toolchain_version cmd
    local -a required_cmds

    distro="$(mbt_detect_distro)"
    mbt_log "doctor report for $SCRIPT_PATH"
    mbt_log "distro family: $distro"

    required_cmds=(git tar wget dfu-util cmake python3 bzip2 lz4 curl ccache ninja)
    if [[ "$distro" == "arch" ]]; then
        required_cmds+=(make)
    fi

    for cmd in "${required_cmds[@]}"; do
        if mbt_command_exists "$cmd"; then
            printf "  [ok] %s\n" "$cmd"
        else
            printf "  [missing] %s\n" "$cmd"
            missing=1
        fi
    done

    toolchain_cc="$SCRIPT_PATH/armbin/bin/arm-none-eabi-gcc"
    if [[ -x "$toolchain_cc" ]]; then
        toolchain_version="$("$toolchain_cc" -dumpfullversion 2>/dev/null || "$toolchain_cc" -dumpversion 2>/dev/null || true)"
        printf "  [ok] arm-none-eabi-gcc %s (%s)\n" "$toolchain_version" "$toolchain_cc"
        if [[ "$toolchain_version" != "9.2.1" ]]; then
            printf "  [warn] expected arm-none-eabi-gcc 9.2.1\n"
        fi
    else
        if mbt_command_exists arm-none-eabi-gcc; then
            toolchain_cc="arm-none-eabi-gcc"
            toolchain_version="$("$toolchain_cc" -dumpfullversion 2>/dev/null || "$toolchain_cc" -dumpversion 2>/dev/null || true)"
            printf "  [ok] arm-none-eabi-gcc %s (%s)\n" "$toolchain_version" "$toolchain_cc"
            if [[ "$toolchain_version" != "9.2.1" ]]; then
                printf "  [warn] expected arm-none-eabi-gcc 9.2.1\n"
            fi
        else
            printf "  [missing] %s\n" "$toolchain_cc"
            missing=1
        fi
    fi

    if git -C "$SCRIPT_PATH" submodule status --recursive | grep -q '^-'; then
        printf "  [warn] some submodules are not initialized (run ./mbt build --with-submodule-sync --device hackrf-one)\n"
    else
        printf "  [ok] submodules initialized\n"
    fi

    if [[ "$missing" -ne 0 ]]; then
        mbt_error "doctor found missing prerequisites."
        return 1
    fi

    mbt_log "doctor passed."
    return 0
}

mbt_validate_jobs() {
    [[ "$1" =~ ^[0-9]+$ ]] && [[ "$1" -gt 0 ]]
}

mbt_detect_cached_device() {
    local build_dir="$1"
    local cache_file="$build_dir/CMakeCache.txt"
    local board flash_limit

    if [[ ! -f "$cache_file" ]]; then
        return 1
    fi

    board="$(grep -E '^BOARD:' "$cache_file" | head -n 1 | cut -d= -f2- || true)"
    flash_limit="$(grep -E '^FLASH_MB_LIMIT_SIZE:' "$cache_file" | head -n 1 | cut -d= -f2- || true)"

    if [[ "$board" == "PRALINE" ]]; then
        printf "hackrf-pro"
        return 0
    fi

    case "$flash_limit" in
    2|2.0)
        printf "portarf"
        return 0
        ;;
    1|1.0)
        printf "hackrf-one"
        return 0
        ;;
    esac

    printf "unknown"
    return 0
}

mbt_clean() {
    local build_dir="$1"
    local libopencm3_dir="$SCRIPT_PATH/hackrf/firmware/libopencm3"

    mbt_log "cleaning build directory: $build_dir"
    if ! rm -rf "$build_dir"; then
        mbt_error "failed to delete build directory. Try running ./mbt --clean as root."
        return 1
    fi

    if [[ -d "$libopencm3_dir" ]]; then
        mbt_log "cleaning libopencm3: $libopencm3_dir"
        make -C "$libopencm3_dir" clean
    else
        mbt_error "libopencm3 directory not found: $libopencm3_dir"
        return 1
    fi
}

mbt_report_build_error() {
    printf '%s\n' 'Build error happened. Try to fix your code, or run ./mbt --clean and try again' >&2
}

mbt_build() {
    local build_dir="$1"
    local generator="$2"
    local use_ccache="$3"
    local device="$4"
    local jobs="$5"
    local target="$6"
    local clean_build="$7"
    shift 7
    if [[ "$#" -lt 2 ]]; then
        mbt_die "internal error: mbt_build expected extra arg array names"
    fi

    local -n cmake_extra_args_ref="$1"
    local -a cmake_extra_args=("${cmake_extra_args_ref[@]}")
    shift
    local -n build_extra_args_ref="$1"
    local -a build_extra_args=("${build_extra_args_ref[@]}")
    local -a cmake_args build_args device_args
    local marker_file="$build_dir/.mbt_device"
    local previous_device=""

    if [[ "$clean_build" -eq 1 ]]; then
        mbt_clean "$build_dir"
        return 0
    else
        if [[ -f "$marker_file" ]]; then
            previous_device="$(<"$marker_file")"
        elif [[ -f "$build_dir/CMakeCache.txt" ]]; then
            previous_device="$(mbt_detect_cached_device "$build_dir" 2>/dev/null || true)"
        fi

        if [[ -n "$previous_device" ]] && [[ "$previous_device" != "$device" ]]; then
            mbt_log "device changed ($previous_device -> $device), cleaning build directory: $build_dir"
            rm -rf "$build_dir"
        fi
    fi

    mkdir -p "$build_dir"

    device_args=()
    case "$device" in
    hackrf-one)
        device_args+=()
        ;;
    hackrf-pro)
        device_args+=("-DBOARD=PRALINE")
        ;;
    portarf)
        device_args+=("-DFLASH_MB_SIZE=2" "-DFLASH_MB_LIMIT_SIZE=2")
        ;;
    *)
        mbt_die "unsupported device preset: $device"
        ;;
    esac

    if [[ "$generator" == "Ninja" ]] && ! mbt_command_exists ninja; then
        if mbt_command_exists make; then
            mbt_log "ninja not found; falling back to Unix Makefiles."
            generator="Unix Makefiles"
        else
            mbt_die "ninja not found and make is unavailable. Install dependencies or set a working generator."
        fi
    fi

    cmake_args=(
        -S "$SCRIPT_PATH"
        -B "$build_dir"
        -G "$generator"
        "-DUSE_CCACHE=$use_ccache"
    )
    cmake_args+=("${device_args[@]}")
    cmake_args+=("${cmake_extra_args[@]}")

    mbt_log "configuring build for $device in $build_dir"
    if ! cmake "${cmake_args[@]}"; then
        mbt_report_build_error
        return 1
    fi
    printf "%s\n" "$device" > "$marker_file"

    build_args=(--build "$build_dir" --parallel "$jobs")
    if [[ -n "$target" ]]; then
        build_args+=(--target "$target")
    fi
    build_args+=("${build_extra_args[@]}")

    mbt_log "building..."
    if ! cmake "${build_args[@]}"; then
        mbt_report_build_error
        return 1
    fi
    mbt_log "build complete. Outputs are in $build_dir."
    mbt_log "The bare binary (for the hackrf_spiflash host tool), excluding external apps, is located at $build_dir/firmware/portapack-mayhem-firmware.bin"
    mbt_log "The full tar.gz package (for MayhemHub and the Flash Utility OTA app), including external apps and the firmware file, is located at $build_dir/firmware/portapack-mayhem_OCI.ppfw.tar"
    mbt_log "If you have the dev's SD card switcher hardware, the fast flash script is at $build_dir/flash.py"
    # have to hard code the name here cuz it's too expensive to read cmake for just a file name.
}

main() {
    local command="build"
    local quickstart=0
    local device=""
    local use_ccache="ON"
    local with_deps=0
    local with_submodule_sync=1
    local with_toolchain=1
    local build_dir="$SCRIPT_PATH/build"
    local generator="Ninja"
    local jobs
    local target=""
    local clean_build=0
    local non_interactive=0
    local -a cmake_extra_args=()
    local -a build_extra_args=()

    FORCE_TOOLCHAIN=0
    jobs="$(mbt_default_jobs)"

    if [[ "$#" -eq 0 ]]; then
        quickstart=1
        with_deps=1
    fi

    if [[ "$#" -gt 0 ]]; then
        case "$1" in
        build|deps|toolchain|doctor|shell)
            command="$1"
            shift
            ;;
        -h|--help|help)
            mbt_print_help
            exit 0
            ;;
        esac
    fi

    while [[ "$#" -gt 0 ]]; do
        case "$1" in
        -d|--device)
            [[ "$#" -ge 2 ]] || mbt_die "$1 requires a value"
            device="$2"
            shift 2
            ;;
        --ccache)
            use_ccache="ON"
            shift
            ;;
        --no-ccache)
            use_ccache="OFF"
            shift
            ;;
        --with-deps)
            with_deps=1
            shift
            ;;
        --no-deps)
            with_deps=0
            shift
            ;;
        --with-submodule-sync)
            with_submodule_sync=1
            shift
            ;;
        --no-submodule-sync)
            with_submodule_sync=0
            shift
            ;;
        --with-toolchain)
            with_toolchain=1
            shift
            ;;
        --no-toolchain)
            with_toolchain=0
            shift
            ;;
        --force-toolchain)
            FORCE_TOOLCHAIN=1
            with_toolchain=1
            shift
            ;;
        -B|--build-dir)
            [[ "$#" -ge 2 ]] || mbt_die "$1 requires a value"
            build_dir="$2"
            shift 2
            ;;
        -G|--generator)
            [[ "$#" -ge 2 ]] || mbt_die "$1 requires a value"
            generator="$2"
            shift 2
            ;;
        -j|--jobs)
            [[ "$#" -ge 2 ]] || mbt_die "$1 requires a value"
            jobs="$2"
            shift 2
            ;;
        --target)
            [[ "$#" -ge 2 ]] || mbt_die "$1 requires a value"
            target="$2"
            shift 2
            ;;
        --clean)
            clean_build=1
            shift
            ;;
        --cmake-arg)
            [[ "$#" -ge 2 ]] || mbt_die "$1 requires a value"
            cmake_extra_args+=("$2")
            shift 2
            ;;
        --cmake-arg=*)
            cmake_extra_args+=("${1#*=}")
            shift
            ;;
        --build-arg)
            [[ "$#" -ge 2 ]] || mbt_die "$1 requires a value"
            build_extra_args+=("$2")
            shift 2
            ;;
        --build-arg=*)
            build_extra_args+=("${1#*=}")
            shift
            ;;
        -y|--non-interactive)
            non_interactive=1
            shift
            ;;
        -h|--help)
            mbt_print_help
            exit 0
            ;;
        *)
            mbt_die "unknown option: $1"
            ;;
        esac
    done

    if ! mbt_validate_jobs "$jobs"; then
        mbt_die "invalid jobs value: $jobs"
    fi

    case "$command" in
    deps)
        mbt_install_deps
        exit 0
        ;;
    toolchain)
        NO_TOOLCHAIN_DOWNLOAD=0
        mbt_enable_toolchain_env
        mbt_log "toolchain ready: $SCRIPT_PATH/armbin"
        exit 0
        ;;
    doctor)
        mbt_doctor
        exit $?
        ;;
    shell)
        if [[ "$with_toolchain" -eq 1 ]]; then
            mbt_enable_toolchain_env
        else
            mbt_use_existing_toolchain_env
        fi
        mbt_log "spawning shell with toolchain exported in this session. Use 'exit' to leave."
        exec "${SHELL:-/bin/bash}" -i
        ;;
    build)
        ;;
    *)
        mbt_die "unsupported command: $command"
        ;;
    esac

    if [[ "$clean_build" -eq 1 ]]; then
        mbt_clean "$build_dir"
        exit 0
    fi

    if [[ "$with_deps" -eq 1 ]]; then
        mbt_install_deps
    fi

    if [[ "$with_submodule_sync" -eq 1 ]]; then
        mbt_sync_submodules
    fi

    if [[ "$with_toolchain" -eq 1 ]]; then
        mbt_enable_toolchain_env
    else
        mbt_use_existing_toolchain_env
    fi

    if [[ -n "$device" ]]; then
        device="$(mbt_normalize_device "$device" 2>/dev/null || true)"
    fi

    if [[ -z "$device" ]]; then
        if [[ "$non_interactive" -eq 1 ]]; then
            mbt_die "--device is required when --non-interactive is set"
        fi
        if [[ ! -t 0 ]]; then
            mbt_die "missing --device and no interactive terminal available"
        fi
        device="$(mbt_prompt_device)" || mbt_die "failed to read device selection"
    fi

    if [[ "$quickstart" -eq 1 ]]; then
        mbt_log "quickstart mode enabled."
    fi

    mbt_build "$build_dir" "$generator" "$use_ccache" "$device" "$jobs" "$target" "$clean_build" \
        cmake_extra_args[@] build_extra_args[@]
}

main "$@"
