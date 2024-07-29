#include "ui_language.hpp"

const char* LanguageHelper::englishMessages[] = {"OK", "Cancel", "Error", "Modem setup", "Debug", "Log", "Done", "Start", "Stop", "Scan", "Clear", "Ready", "Data:", "Loop", "Reset"};

const char** LanguageHelper::currentMessages = englishMessages;

void LanguageHelper::setLanguage(LanguageList lang) {
    switch (lang) {
        default:
        case ENGLISH:
            currentMessages = englishMessages;
            break;
    }
}

const char* LanguageHelper::getMessage(LangConsts msg) {
    return currentMessages[msg];
}
