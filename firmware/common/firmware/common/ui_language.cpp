#include "ui_language.hpp"
#include "lang_tr.hpp"

const char* LanguageHelper::englishMessages[] = {
    "OK", "Cancel", "Error", "Modem setup", "Debug", "Log", "Done", "Start", "Stop", "Scan", "Clear", "Ready", "Data:", "Loop", "Reset", "Pause", "Resume", "Flood", "Show QR", "Save", "Lock", "Unlock", "Browse", "Set", "Open File", "Save File", "Send", "Receive"
};
const char* LanguageHelper::turkishMessages[] = turkishMessages;

const char** LanguageHelper::currentMessages = englishMessages;

void LanguageHelper::setLanguage(LanguageList lang) {
    switch (lang) {
        case TURKISH:
            currentMessages = turkishMessages;
            break;
        default:
        case ENGLISH:
            currentMessages = englishMessages;
            break;
    }
}

const char* LanguageHelper::getMessage(LangConsts msg) {
    return currentMessages[msg];
}