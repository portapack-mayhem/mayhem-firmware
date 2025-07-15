#include "ui_language.hpp"
// ... diğer gerekli include’lar ve kodlar ...

// Eğer bir ayarlar menü sistemi varsa, aşağıdaki gibi bir dil seçimi ekleyin:

const char* languageOptions[] = { "English", "Türkçe" };

MenuEntry languageMenuEntry;
languageMenuEntry.title = "Language";
languageMenuEntry.type = MENU_ENTRY_TYPE_CHOICE;
languageMenuEntry.choices = languageOptions;
languageMenuEntry.numChoices = 2;
languageMenuEntry.selected = (LanguageHelper::currentMessages == LanguageHelper::englishMessages) ? 0 : 1;
languageMenuEntry.onChange = [](int choice) {
    if (choice == 0) LanguageHelper::setLanguage(ENGLISH);
    else if (choice == 1) LanguageHelper::setLanguage(TURKISH);
    // Menülerin yeniden çizilmesi gerekiyorsa burada çağırın (ör: refreshMenu();)
};
// Menüye ekleyin:
settingsMenu.addEntry(languageMenuEntry);