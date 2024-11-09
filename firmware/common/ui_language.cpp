#include "ui_language.hpp"

// use the exact position in this array! the enum's value is the identifier. Best to add to the end
const char* LanguageHelper::englishMessages[] = {"OK", "Cancel", "Error", "Modem setup", "Debug", "Log", "Done", "Start", "Stop", "Scan", "Clear", "Ready", "Data:", "Loop", "Reset", "Pause", "Resume", "Flood", "Show QR", "Save", "Lock", "Unlock"};

// multi language support will changes (not in use for now)
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
