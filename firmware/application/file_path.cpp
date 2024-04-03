/*
 * Copyright (C) 2024 Mark Thompson
 * copyleft (ɔ) 2024 zxkmm with the GPL license
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "file_path.hpp"
#include "file.hpp"

const std::filesystem::path adsb_dir = u"SYS/ADSB";  // this dir no need to seperate profiles since all are resources

const std::filesystem::path ais_dir = u"SYS/AIS";  // this dir no need to seperate profiles since all are resources

const std::filesystem::path apps_dir = u"APPS";  // currently in root

// APRS
const std::filesystem::path aprs_dir = u"APRS";  // fake path
const std::filesystem::path aprs_dir_user = u"/APRS";
const std::filesystem::path aprs_dir_resources = u"SYS/APRS";

// AUDIO
const std::filesystem::path audio_dir = u"AUDIO";  // fake path
const std::filesystem::path audio_dir_user = u"/AUDIO";
const std::filesystem::path audio_dir_resources = u"SYS/AUDIO";

// BLERX
const std::filesystem::path blerx_dir = u"BLERX";  // fake path
const std::filesystem::path blerx_dir_user = u"/BLERX";
const std::filesystem::path blerx_dir_resources = u"SYS/BLERX";

// BLETX
const std::filesystem::path bletx_dir = u"BLETX";  // fake path
const std::filesystem::path bletx_dir_user = u"/BLETX";
const std::filesystem::path bletx_dir_resources = u"SYS/BLETX";

// CAPTURES
const std::filesystem::path captures_dir = u"CAPTURES";        // fake path
const std::filesystem::path captures_dir_user = u"/CAPTURES";  // used by recon and capture apps to save capture C16 into USR
const std::filesystem::path captures_dir_resources = u"SYS/CAPTURES";

const std::filesystem::path debug_dir = u"DEBUG";  // in root

const std::filesystem::path firmware_dir = u"FIRMWARE";  // in root

// FREQMAN
const std::filesystem::path freqman_dir = u"FREQMAN";  // fake path
const std::filesystem::path freqman_dir_user = u"/FREQMAN";
const std::filesystem::path freqman_dir_resources = u"SYS/FREQMAN";

const std::filesystem::path fskrx_dir_user = u"/FSKRX";

const std::filesystem::path gps_dir = u"/GPS";

const std::filesystem::path logs_dir = u"/LOGS";

const std::filesystem::path looking_glass_dir = u"SYS/LOOKINGGLASS";

// PLAYLIST
const std::filesystem::path playlist_dir = u"PLAYLIST";                // fake dir
const std::filesystem::path playlist_dir_user = u"/PLAYLIST";          // used by playlist aka replay app, to save nre created PPL files into USR
const std::filesystem::path playlist_dir_resources = u"SYS/PLAYLIST";  //

// REMOTE
const std::filesystem::path remotes_dir = u"REMOTES";  // fake dir
const std::filesystem::path remotes_dir_user = u"/REMOTES";
const std::filesystem::path remotes_dir_resources = u"SYS/REMOTES";

// recon app
const std::filesystem::path repeat_rec_path = u"/CAPTURES";  // this is for recon captures files

const std::filesystem::path samples_dir = u"SYS/CAPTURES";  // TODO: rename var name

const std::filesystem::path screenshots_dir = u"/SCREENSHOTS";  // TODO: rename var name to screenshots_dir

const std::filesystem::path settings_dir = u"/SETTINGS";

const std::filesystem::path spectrum_dir = u"SPECTRUM";  // fake dir

const std::filesystem::path splash_dir = u"/SPLASH";

const std::filesystem::path sstv_dir = u"SSTV";  // fake path
const std::filesystem::path sstv_dir_user = u"/SSTV";
const std::filesystem::path sstv_dir_resources = u"SYS/SSTV";

const std::filesystem::path system_dir = u"/SYS";  // in root
const std::filesystem::path user_dir = u"/";       // root

// WAV
const std::filesystem::path wav_dir = u"WAV";  // fake path
const std::filesystem::path wav_dir_user = u"/WAV";
const std::filesystem::path wav_dir_resources = u"SYS/WAV";

const std::filesystem::path whipcalc_dir = u"SYS/WHIPCALC";

// vector

const std::vector<std::string> allow_dirs = {
    "ADSB",
    "AIS",
    "BLETX",
    "GPS",
    "LOOKINGGLASS",
    "PLAYLIST",
    "REMOTES",
    "SPLASH",
    "SSTV",
    "WAV",
    "WHIPCALC",
    "SAMPLES",
};
