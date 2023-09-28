#include "ui_external_items_menu_loader.hpp"

#include "external_app.hpp"

namespace ui {

std::vector<GridItem> ExternalItemsMenuLoader::load_external_items(app_location_t app_location, NavigationView& nav) {
    // TODO: load listfrom sd card

    if (app_location == app_location_t::UTILITIES) {
        return {
            {"AFSK", Color::dark_blue(), &bitmap_icon_morse, [&nav, app_location]() {
                 FIL firmware_file;
                 size_t bytes_read;

                 if (f_open(&firmware_file, reinterpret_cast<const TCHAR*>(u"/application_ext_afsk_rx_patched.bin"), FA_READ) != FR_OK)
                     chDbgPanic("no file");

                 if (f_read(&firmware_file, &shared_memory.bb_data.data[0], sizeof(application_information_t), &bytes_read) != FR_OK)
                     chDbgPanic("no data");

                 f_lseek(&firmware_file, 0);

                 application_information_t* application_information = (application_information_t*)&shared_memory.bb_data.data[0];

                 if ((uint32_t)application_information->memory_location != 0x10086000)
                     chDbgPanic("memory_location");

                 if (application_information->location != app_location)
                     return;

                 for (size_t page_index = 0; page_index < 64 * 512; page_index += 512) {
                     if (f_read(&firmware_file, application_information->memory_location + page_index, 512, &bytes_read) != FR_OK)
                         chDbgPanic("no data #2");

                     if (bytes_read < 512)
                         break;
                 }

                 // void (*f)(void*) = (void (*)(void*))( | 0x01 /*Thumb mode*/);
                 void* p = nullptr;
                 application_information->externalAppEntry(nav, &p);

                 if (p == nullptr)
                     chDbgPanic("memory_location");

                 ui::View* obj = reinterpret_cast<ui::View*>(p);
                 // auto str = obj->title();
                 // chDbgPanic(str.c_str());
                 // ui::View* obj = dynamic_cast<ui::View*>(p);
                 // ui::View* obj = static_cast<ui::View*>(p);
                 // ui::View* obj = (ui::View*)p;

                 nav.push_view(std::unique_ptr<ui::View>(obj));
             }},
            {"Pacman", Color::dark_blue(), &bitmap_icon_morse, [&nav, app_location]() {
                 FIL firmware_file;
                 size_t bytes_read;

                 if (f_open(&firmware_file, reinterpret_cast<const TCHAR*>(u"/application_ext_pacman_patched.bin"), FA_READ) != FR_OK)
                     chDbgPanic("no file");

                 if (f_read(&firmware_file, &shared_memory.bb_data.data[0], sizeof(application_information_t), &bytes_read) != FR_OK)
                     chDbgPanic("no data");

                 f_lseek(&firmware_file, 0);

                 application_information_t* application_information = (application_information_t*)&shared_memory.bb_data.data[0];

                 if ((uint32_t)application_information->memory_location != 0x10080000)
                     chDbgPanic("memory_location");

                 if (application_information->location != app_location)
                     return;

                 for (size_t page_index = 0; page_index < 64 * 512; page_index += 512) {
                     if (f_read(&firmware_file, application_information->memory_location + page_index, 512, &bytes_read) != FR_OK)
                         chDbgPanic("no data #2");

                     if (bytes_read < 512)
                         break;
                 }

                 // void (*f)(void*) = (void (*)(void*))( | 0x01 /*Thumb mode*/);
                 void* p = nullptr;
                 application_information->externalAppEntry(nav, &p);

                 if (p == nullptr)
                     chDbgPanic("memory_location");

                 ui::View* obj = reinterpret_cast<ui::View*>(p);
                 // auto str = obj->title();
                 // chDbgPanic(str.c_str());
                 // ui::View* obj = dynamic_cast<ui::View*>(p);
                 // ui::View* obj = static_cast<ui::View*>(p);
                 // ui::View* obj = (ui::View*)p;

                 nav.push_view(std::unique_ptr<ui::View>(obj));
             }},
        };
    } else {
        return {};
    }
}
}  // namespace ui
