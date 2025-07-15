/*
 * Copyright (C) 2024 HTotoo
 * ... (mevcut telif ve açıklamalar) ...
 */

#ifndef __UI_LANGUAGE_H__
#define __UI_LANGUAGE_H__

// Çoklu dil desteği için güncellendi
enum LanguageList {
    ENGLISH,
    TURKISH,
};

// enum'a eleman ekledikçe, ilgili string'i LanguageHelper'da aynı sıraya ekleyin
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
    LANG_UNLOCK,
    LANG_BROWSE,
    LANG_SET,
    LANG_OPEN_FILE,
    LANG_SAVE_FILE,
    LANG_SEND,
    LANG_RECV
};

class LanguageHelper {
   public:
    static void setLanguage(LanguageList lang);
    static const char* getMessage(LangConsts msg);
    static const char** currentMessages;

   private:
    static const char* englishMessages[];
    static const char* turkishMessages[];
};

#endif