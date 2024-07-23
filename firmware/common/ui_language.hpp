#ifndef __UI_LANGUAGE_H__
#define __UI_LANGUAGE_H__

enum LanguageList {
    ENGLISH,
};

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
    LANG_RESET
};

class LanguageHelper {
   public:
    static void setLanguage(LanguageList lang);
    static const char* getMessage(LangConsts msg);
    static const char** currentMessages;  // expose, so can link directly too

   private:
    static const char* englishMessages[];
};

#endif