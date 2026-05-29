#!/bin/sh

# shellcheck disable=SC2039

MBT_ROOT="${MBT_ROOT:-$(pwd -P)}"
MBT_VERBOSE="${MBT_VERBOSE:-}"
MBT_TOOLCHAIN_DIR="${MBT_TOOLCHAIN_DIR:-$MBT_ROOT/armbin}"
MBT_TOOLCHAIN_URL="${MBT_TOOLCHAIN_URL:-https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2019q4/RC2.1/gcc-arm-none-eabi-9-2019-q4-major-x86_64-linux.tar.bz2?revision=6e63531f-8cb1-40b9-bbfc-8a57cdfc01b4&la=en&hash=F761343D43A0587E8AC0925B723C04DBFB848339}"
MBT_EXPECTED_GCC_VERSION="${MBT_EXPECTED_GCC_VERSION:-9.2.1}"
MBT_FORCE_TOOLCHAIN="${MBT_FORCE_TOOLCHAIN:-0}"
MBT_NO_TOOLCHAIN_DOWNLOAD="${MBT_NO_TOOLCHAIN_DOWNLOAD:-0}"
MBT_TOOLCHAIN_BIN="$MBT_TOOLCHAIN_DIR/bin"
MBT_TOOLCHAIN_CC="$MBT_TOOLCHAIN_BIN/arm-none-eabi-gcc"
MBT_TMP_ARCHIVE=""
MBT_DOWNLOADER=""

mbtenv_log() {
    printf "mbt: %s\n" "$*"
}

mbtenv_verbose() {
    if [ -n "$MBT_VERBOSE" ]; then
        mbtenv_log "$*"
    fi
}

mbtenv_command_exists() {
    command -v "$1" >/dev/null 2>&1
}

mbtenv_cleanup_tmp() {
    if [ -n "$MBT_TMP_ARCHIVE" ] && [ -f "$MBT_TMP_ARCHIVE" ]; then
        rm -f "$MBT_TMP_ARCHIVE"
    fi
}

mbtenv_check_linux() {
    if [ "$(uname -s)" != "Linux" ]; then
        mbtenv_log "error: mbt currently supports Linux only."
        return 1
    fi

    arch="$(uname -m)"
    case "$arch" in
        x86_64|amd64)
            # Supported architecture for the default x86_64 Linux toolchain archive.
            ;;
        *)
            mbtenv_log "error: unsupported architecture '$arch'. mbt currently supports x86_64 Linux only for the bundled toolchain."
            return 1
            ;;
    esac
    return 0
}

mbtenv_choose_downloader() {
    if mbtenv_command_exists curl; then
        MBT_DOWNLOADER="curl"
        return 0
    fi

    if mbtenv_command_exists wget; then
        MBT_DOWNLOADER="wget"
        return 0
    fi

    mbtenv_log "error: neither curl nor wget found in PATH."
    return 1
}

mbtenv_download_archive() {
    if [ "$MBT_DOWNLOADER" = "curl" ]; then
        curl --fail --location --progress-bar --output "$1" "$2"
        return $?
    fi

    wget --show-progress --progress=bar:force -O "$1" "$2"
}

mbtenv_get_installed_version() {
    if [ ! -x "$MBT_TOOLCHAIN_CC" ]; then
        return 1
    fi

    "$MBT_TOOLCHAIN_CC" -dumpfullversion 2>/dev/null || "$MBT_TOOLCHAIN_CC" -dumpversion 2>/dev/null
}

mbtenv_toolchain_is_expected() {
    INSTALLED_VERSION="$(mbtenv_get_installed_version 2>/dev/null || true)"
    if [ "$INSTALLED_VERSION" = "$MBT_EXPECTED_GCC_VERSION" ]; then
        return 0
    fi
    return 1
}

mbtenv_download_and_unpack_toolchain() {
    mbtenv_choose_downloader || return 1

    MBT_TMP_ARCHIVE="$(mktemp /tmp/mbt-toolchain-XXXXXX.tar.bz2)"
    trap mbtenv_cleanup_tmp EXIT INT TERM

    mbtenv_log "downloading ARM GNU toolchain..."
    mbtenv_download_archive "$MBT_TMP_ARCHIVE" "$MBT_TOOLCHAIN_URL" || {
        mbtenv_log "error: failed to download toolchain from ARM."
        return 1
    }

    mbtenv_log "putting toolchain into $MBT_TOOLCHAIN_DIR"
    if [ -d "$MBT_TOOLCHAIN_DIR" ] && [ ! -f "$MBT_TOOLCHAIN_DIR/bin/arm-none-eabi-gcc-9.2.1" ]; then
        mbtenv_log "Sorry, i can't verify if the toolchain dir is legit to delete. it either could be becasue you failed to download the full toolchain last time, or the enviroment var were pointed to something else, which is not safe for me to remove. please check if it's safe to remove $MBT_TOOLCHAIN_DIR and manually remove $MBT_TOOLCHAIN_DIR"
        return 1
    fi
    rm -rf "$MBT_TOOLCHAIN_DIR"
    mkdir -p "$MBT_TOOLCHAIN_DIR"
    tar --strip-components=1 -xjf "$MBT_TMP_ARCHIVE" -C "$MBT_TOOLCHAIN_DIR" || {
        mbtenv_log "error: failed to extract toolchain archive."
        return 1
    }

    mbtenv_cleanup_tmp
    trap - EXIT INT TERM
    MBT_TMP_ARCHIVE=""
    return 0
}

mbtenv_ensure_toolchain() {
    if mbtenv_toolchain_is_expected && [ "$MBT_FORCE_TOOLCHAIN" != "1" ]; then
        mbtenv_verbose "toolchain already installed at $MBT_TOOLCHAIN_DIR"
        return 0
    fi

    if [ "$MBT_NO_TOOLCHAIN_DOWNLOAD" = "1" ]; then
        if [ ! -x "$MBT_TOOLCHAIN_CC" ]; then
            mbtenv_log "error: arm-none-eabi-gcc not found in $MBT_TOOLCHAIN_BIN"
            mbtenv_log "hint: run ./mbt toolchain once, or remove --no-toolchain."
            return 1
        fi

        INSTALLED_VERSION="$(mbtenv_get_installed_version 2>/dev/null || true)"
        mbtenv_log "error: found arm-none-eabi-gcc $INSTALLED_VERSION, expected $MBT_EXPECTED_GCC_VERSION"
        mbtenv_log "hint: run ./mbt toolchain once, or remove --no-toolchain."
        return 1
    fi

    if [ -x "$MBT_TOOLCHAIN_CC" ]; then
        INSTALLED_VERSION="$(mbtenv_get_installed_version 2>/dev/null || true)"
        mbtenv_log "toolchain version mismatch (found $INSTALLED_VERSION, expected $MBT_EXPECTED_GCC_VERSION)"
        mbtenv_log "reinstalling toolchain..."
    fi

    mbtenv_download_and_unpack_toolchain || return 1

    if ! mbtenv_toolchain_is_expected; then
        INSTALLED_VERSION="$(mbtenv_get_installed_version 2>/dev/null || true)"
        mbtenv_log "error: installed toolchain version check failed (found $INSTALLED_VERSION)."
        return 1
    fi
    return 0
}

mbtenv_activate_path() {
    if [ ! -d "$MBT_TOOLCHAIN_BIN" ]; then
        mbtenv_log "error: missing toolchain bin directory: $MBT_TOOLCHAIN_BIN"
        return 1
    fi

    case ":$PATH:" in
    *":$MBT_TOOLCHAIN_BIN:"*)
        return 0
        ;;
    esac

    if [ -z "${MBT_SAVED_PATH+x}" ]; then
        MBT_SAVED_PATH="$PATH"
        export MBT_SAVED_PATH
    fi

    PATH="$MBT_TOOLCHAIN_BIN:$PATH"
    export PATH
    return 0
}

mbtenv_restore() {
    if [ -n "${MBT_SAVED_PATH+x}" ]; then
        PATH="$MBT_SAVED_PATH"
        export PATH
        unset MBT_SAVED_PATH
    fi
    return 0
}

mbtenv_main() {
    mbtenv_check_linux || return 1

    if [ "${1:-}" = "--restore" ]; then
        mbtenv_restore
        return 0
    fi

    if [ "${MBT_NOENV:-0}" = "1" ]; then
        return 0
    fi

    mbtenv_ensure_toolchain || return 1
    mbtenv_activate_path || return 1
    return 0
}

mbtenv_main "${1:-}"
