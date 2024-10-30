/*
 * Copyright (C) 2024 HTotoo
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

/*

The purpose of this: when creating an ext app it's address will be changed. But the compiler doesn't know this, so when it optimizes literals it can remove something from one ext app, that is included in the other, to save space.
But when we want to run this app, it won't find the literal at the fake address, so the best case we'll get just garbage or nothing instead of the string.
So we must use this Language helper that stores the literals in the fw instead of any ext app, so it'll be always available.
In the first run we must move those string literal here what are not so unique, like "OK".
*/

#ifndef __UI_LANGUAGE_H__
#define __UI_LANGUAGE_H__

// For future multi language support
enum LanguageList {
    ENGLISH,
};

// Add an element to this enum when you fill a new value.
// Then add the string literal to the LanguageHelper::englishMessages at the exact position where the enum points to (usualy to the end)

enum LangConsts {
    LANG_OK,
    LANG_CANCEL,
    LANG_ERROR,
    LANG_MODEM_SETUP,
    LANG_DEBUG,
    LANG_LOG,
    LANG_DONE,
    LANG_START,
    LANG_STOP,
    LANG_SCAN,
    LANG_CLEAR,
    LANG_READY,
    LANG_DATADP,
    LANG_LOOP,
    LANG_RESET,
    LANG_PAUSE,
    LANG_RESUME,
    LANG_FLOOD,
    LANG_SHOWQR,
    LANG_SAVE,
    LANG_LOCK,
    LANG_UNLOCK 
};

class LanguageHelper {
   public:
    static void setLanguage(LanguageList lang);  // changes the pointer to the currently used language literal container. not used yet.
    static const char* getMessage(LangConsts msg);
    static const char** currentMessages;  // expose, so can link directly too

   private:
    static const char* englishMessages[];
};

#endif