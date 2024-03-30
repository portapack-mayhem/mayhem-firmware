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

const std::filesystem::path adsb_dir = u"RES/ADSB";  // this dir no need to seperate profiles since all are resources

const std::filesystem::path ais_dir = u"RES/AIS";  // this dir no need to seperate profiles since all are resources

const std::filesystem::path apps_dir = u"APPS";  // currently in root

// APRS
const std::filesystem::path aprs_dir = u"APRS";  // fake path
const std::filesystem::path aprs_dir_user = u"USR/APRS";
const std::filesystem::path aprs_dir_resources = u"RES/APRS";

// AUDIO
const std::filesystem::path audio_dir = u"AUDIO";
const std::filesystem::path audio_dir_user = u"USR/AUDIO";
const std::filesystem::path audio_dir_resources = u"RES/AUDIO";

// BLERX
const std::filesystem::path blerx_dir = u"BLERX";
const std::filesystem::path blerx_dir_user = u"USR/BLERX";
const std::filesystem::path blerx_dir_resources = u"RES/BLERX";

// BLETX
const std::filesystem::path bletx_dir = u"BLETX";
const std::filesystem::path bletx_dir_user = u"USR/BLETX";
const std::filesystem::path bletx_dir_resources = u"RES/BLETX";

// CAPTURES
const std::filesystem::path captures_dir = u"CAPTURES";
const std::filesystem::path captures_dir_user = u"USR/CAPTURES";  // used by recon and capture apps to save capture C16 into USR
const std::filesystem::path captures_dir_resources = u"RES/CAPTURES";

const std::filesystem::path debug_dir = u"DEBUG";  // in root

const std::filesystem::path firmware_dir = u"FIRMWARE";  // in root

// FREQMAN
const std::filesystem::path freqman_dir = u"FREQMAN";
const std::filesystem::path freqman_dir_user = u"USR/FREQMAN";
const std::filesystem::path freqman_dir_resources = u"RES/FREQMAN";

const std::filesystem::path gps_dir = u"USR/GPS";

const std::filesystem::path logs_dir = u"USR/LOGS";

const std::filesystem::path looking_glass_dir = u"RES/LOOKINGGLASS";

// PLAYLIST
const std::filesystem::path playlist_dir = u"PLAYLIST";                // fake dir
const std::filesystem::path playlist_dir_user = u"USR/PLAYLIST";       // used by playlist aka replay app, to save nre created PPL files into USR
const std::filesystem::path playlist_dir_resources = u"RES/PLAYLIST";  //

// REMOTE
const std::filesystem::path remotes_dir = u"REMOTES";  // fake dir
const std::filesystem::path remotes_dir_user = u"USR/REMOTES";
const std::filesystem::path remotes_dir_resources = u"RES/REMOTES";

// recon app
const std::filesystem::path repeat_rec_path = u"USR/CAPTURES";  // this is for recon captures files

const std::filesystem::path samples_dir = u"RES/CAPTURES";  // TODO: rename var name

const std::filesystem::path screenshots_dir = u"USR/SCREENSHOTS";  // TODO: rename var name to screenshots_dir

const std::filesystem::path settings_dir = u"USR/SETTINGS";

const std::filesystem::path spectrum_dir = u"SPECTRUM";  // fake dir

const std::filesystem::path splash_dir = u"USR/SPLASH";

const std::filesystem::path sstv_dir = u"SSTV";
const std::filesystem::path sstv_dir_user = u"USR/SSTV";
const std::filesystem::path sstv_dir_resources = u"USR/SSTV";

// WAV
const std::filesystem::path wav_dir = u"WAV";
const std::filesystem::path wav_dir_user = u"USR/WAV";
const std::filesystem::path wav_dir_resources = u"RES/WAV";

const std::filesystem::path whipcalc_dir = u"RES/WHIPCALC";
