#include "ui_external_items_menu_loader.hpp"

#include "sd_card.hpp"

namespace ui {

std::vector<Bitmap*> ExternalItemsMenuLoader::bitmaps;

std::vector<GridItem> ExternalItemsMenuLoader::load_external_items(app_location_t app_location, NavigationView& nav) {
    for (const auto& bitmap : bitmaps) {
        delete bitmap->data;
        delete bitmap;
    }
    bitmaps.clear();

    std::vector<GridItem> exteral_apps;

    if (sd_card::status() != sd_card::Status::Mounted)
        return exteral_apps;

    for (const auto& entry : std::filesystem::directory_iterator(u"APPS", u"*.ppma")) {
        FIL ppma_file;
        auto filePath = u"/APPS/" + entry.path().native();
        if (f_open(&ppma_file, reinterpret_cast<const TCHAR*>(filePath.c_str()), FA_READ) != FR_OK)
            continue;

        size_t bytes_read;
        application_information_t application_information = {};
        if (f_read(&ppma_file, &application_information, sizeof(application_information_t), &bytes_read) != FR_OK) continue;

        if (application_information.menu_location != app_location)
            continue;

        GridItem gridItem = {};
        gridItem.text = reinterpret_cast<char*>(&application_information.app_name[0]);
        gridItem.color = Color((uint16_t)application_information.icon_color);

        auto bitmapData = new uint8_t[32];
        Bitmap* bitmap = new Bitmap{
            {16, 16},
            bitmapData};

        memcpy(bitmapData, &application_information.bitmap_data[0], 32);
        bitmaps.push_back(bitmap);

        gridItem.bitmap = bitmap;
        gridItem.on_select = [&nav, app_location, filePath]() {
            auto filename = reinterpret_cast<const TCHAR*>(filePath.c_str());
            run_external_app(nav, filename);
        };

        exteral_apps.push_back(gridItem);
    }

    return exteral_apps;
}

void ExternalItemsMenuLoader::run_external_app(ui::NavigationView& nav, const TCHAR* filename) {
    FIL firmware_file;
    size_t bytes_read;

    if (f_open(&firmware_file, filename, FA_READ) != FR_OK)
        chDbgPanic("no file");

    if (f_read(&firmware_file, &shared_memory.bb_data.data[0], sizeof(application_information_t), &bytes_read) != FR_OK)
        chDbgPanic("no data");

    f_lseek(&firmware_file, 0);

    application_information_t* application_information = (application_information_t*)&shared_memory.bb_data.data[0];

    // copy application image
    for (size_t file_read_index = 0; file_read_index < application_information->m4_app_offset; file_read_index += 512) {
        auto bytes_to_read = 512;
        if (file_read_index + 512 > application_information->m4_app_offset)
            bytes_to_read = application_information->m4_app_offset - file_read_index;

        if (bytes_to_read == 0)
            break;

        if (f_read(&firmware_file, &application_information->memory_location[file_read_index], bytes_to_read, &bytes_read) != FR_OK)
            chDbgPanic("no data #2");

        if (bytes_read < 512)
            break;
    }

    // copy baseband image
    for (size_t file_read_index = application_information->m4_app_offset;; file_read_index += bytes_read) {
        size_t bytes_to_read = 512;

        // not aligned
        if ((file_read_index % 512) != 0)
            bytes_to_read = 512 - (file_read_index % 512);

        if (bytes_to_read == 0)
            break;

        auto target_memory = reinterpret_cast<void*>(portapack::memory::map::m4_code.base() + file_read_index - application_information->m4_app_offset);
        if (f_read(&firmware_file, target_memory, bytes_to_read, &bytes_read) != FR_OK)
            chDbgPanic("no data #2");

        if (bytes_read != bytes_to_read)
            break;
    }

    application_information->externalAppEntry(nav);
}

}  // namespace ui
