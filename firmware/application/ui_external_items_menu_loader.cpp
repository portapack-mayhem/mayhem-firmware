#include "ui_external_items_menu_loader.hpp"

#include "sd_card.hpp"

namespace ui {

std::vector<Bitmap*> ExternalItemsMenuLoader::bitmaps;

std::vector<GridItem> ExternalItemsMenuLoader::load_external_items(app_location_t app_location, NavigationView& nav) {
    for (const auto& bitmap : bitmaps) {
        delete[] bitmap->data;
        delete bitmap;
    }
    bitmaps.clear();

    std::vector<GridItem> external_apps;

    if (sd_card::status() != sd_card::Status::Mounted)
        return external_apps;

    for (const auto& entry : std::filesystem::directory_iterator(u"APPS", u"*.ppma")) {
        auto filePath = u"/APPS/" + entry.path();
        File app;

        auto openError = app.open(filePath);
        if (openError)
            continue;

        application_information_t application_information = {};

        auto readResult = app.read(&application_information, sizeof(application_information_t));
        if (!readResult)
            continue;

        if (application_information.menu_location != app_location)
            continue;

        if (application_information.header_version != CURRENT_HEADER_VERSION)
            continue;

        bool versionMatches = VERSION_MD5 == application_information.app_version;

        GridItem gridItem = {};
        gridItem.text = reinterpret_cast<char*>(&application_information.app_name[0]);

        if (versionMatches) {
            gridItem.color = Color((uint16_t)application_information.icon_color);

            auto bitmapData = new uint8_t[32];
            Bitmap* bitmap = new Bitmap{
                {16, 16},
                bitmapData};

            memcpy(bitmapData, &application_information.bitmap_data[0], 32);
            bitmaps.push_back(bitmap);

            gridItem.bitmap = bitmap;

            gridItem.on_select = [&nav, app_location, filePath]() {
                run_external_app(nav, filePath);
            };
        } else {
            gridItem.color = Color::light_grey();

            gridItem.bitmap = &bitmap_sd_card_error;

            gridItem.on_select = [&nav]() {
                nav.display_modal("Error", "The .ppma file in your APPS\nfolder is outdated. Please\nupdate your SD Card content.");
            };
        }

        external_apps.push_back(gridItem);
    }

    return external_apps;
}

void ExternalItemsMenuLoader::run_external_app(ui::NavigationView& nav, std::filesystem::path filePath) {
    File app;

    auto openError = app.open(filePath);
    if (openError)
        chDbgPanic("file gone");

    application_information_t application_information = {};

    auto readResult = app.read(&application_information, sizeof(application_information_t));

    if (!readResult)
        chDbgPanic("no data");

    app.seek(0);

    if (application_information.m4_app_offset != 0) {
        // copy application image
        for (size_t file_read_index = 0; file_read_index < application_information.m4_app_offset; file_read_index += std::filesystem::max_file_block_size) {
            auto bytes_to_read = std::filesystem::max_file_block_size;
            if (file_read_index + std::filesystem::max_file_block_size > application_information.m4_app_offset)
                bytes_to_read = application_information.m4_app_offset - file_read_index;

            if (bytes_to_read == 0)
                break;

            readResult = app.read(&application_information.memory_location[file_read_index], bytes_to_read);
            if (!readResult)
                chDbgPanic("read error");

            if (readResult.value() < std::filesystem::max_file_block_size)
                break;
        }

        // copy baseband image
        for (size_t file_read_index = application_information.m4_app_offset;; file_read_index += readResult.value()) {
            size_t bytes_to_read = std::filesystem::max_file_block_size;

            // not aligned
            if ((file_read_index % std::filesystem::max_file_block_size) != 0)
                bytes_to_read = std::filesystem::max_file_block_size - (file_read_index % std::filesystem::max_file_block_size);

            if (bytes_to_read == 0)
                break;

            auto target_memory = reinterpret_cast<void*>(portapack::memory::map::m4_code.base() + file_read_index - application_information.m4_app_offset);

            readResult = app.read(target_memory, bytes_to_read);
            if (!readResult)
                chDbgPanic("read error #2");

            if (readResult.value() != bytes_to_read)
                break;
        }
    } else {
        // copy application image
        for (size_t file_read_index = 0; file_read_index < 80 * std::filesystem::max_file_block_size; file_read_index += std::filesystem::max_file_block_size) {
            auto bytes_to_read = std::filesystem::max_file_block_size;

            readResult = app.read(&application_information.memory_location[file_read_index], bytes_to_read);
            if (!readResult)
                chDbgPanic("read error #3");

            if (readResult.value() < std::filesystem::max_file_block_size)
                break;
        }
    }

    application_information.externalAppEntry(nav);
}

}  // namespace ui
