#include "ui_external_items_menu_loader.hpp"

namespace ui {

std::vector<GridItem> ExternalItemsMenuLoader::load_external_items(app_location_t app_location, NavigationView& nav) {
    // TODO: load listfrom sd card

    if (app_location == app_location_t::UTILITIES) {
        return {
            {"AFSK", Color::dark_blue(), &bitmap_icon_modem, [&nav, app_location]() {
                 FIL firmware_file;
                 size_t bytes_read;

                 if (f_open(&firmware_file, reinterpret_cast<const TCHAR*>(u"/external_app_afsk_rx.ppma"), FA_READ) != FR_OK)
                     chDbgPanic("no file");

                 if (f_read(&firmware_file, &shared_memory.bb_data.data[0], sizeof(application_information_t), &bytes_read) != FR_OK)
                     chDbgPanic("no data");

                 f_lseek(&firmware_file, 0);

                 application_information_t* application_information = (application_information_t*)&shared_memory.bb_data.data[0];

                 for (size_t page_index = 0; page_index < application_information->m4_app_offset; page_index += 512) {
                     auto bytes_to_read = 512;
                     if (page_index + 512 > application_information->m4_app_offset)
                         bytes_to_read = application_information->m4_app_offset - page_index;

                     if (bytes_to_read == 0)
                         break;

                     if (f_read(&firmware_file, &application_information->memory_location[page_index], bytes_to_read, &bytes_read) != FR_OK)
                         chDbgPanic("no data #2");

                     if (bytes_read < 512)
                         break;
                 }

                 application_information->externalAppEntry(nav);
             }},
            {"Pacman", Color::dark_blue(), &bitmap_icon_morse, [&nav, app_location]() {
                 FIL firmware_file;
                 size_t bytes_read;

                 if (f_open(&firmware_file, reinterpret_cast<const TCHAR*>(u"/external_app_pacman.ppma"), FA_READ) != FR_OK)
                     chDbgPanic("no file");

                 if (f_read(&firmware_file, &shared_memory.bb_data.data[0], sizeof(application_information_t), &bytes_read) != FR_OK)
                     chDbgPanic("no data");

                 f_lseek(&firmware_file, 0);

                 application_information_t* application_information = (application_information_t*)&shared_memory.bb_data.data[0];

                 for (size_t page_index = 0; page_index < 64 * 512; page_index += 512) {
                     if (f_read(&firmware_file, &application_information->memory_location[page_index], 512, &bytes_read) != FR_OK)
                         chDbgPanic("no data #2");

                     if (bytes_read < 512)
                         break;
                 }

                 application_information->externalAppEntry(nav);
             }},
        };
    } else {
        return {

        };
    }
}
}  // namespace ui
