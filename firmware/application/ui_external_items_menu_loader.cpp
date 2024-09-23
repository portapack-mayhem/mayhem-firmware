#include "ui_external_items_menu_loader.hpp"

#include "sd_card.hpp"
#include "file_path.hpp"
#include "ui_standalone_view.hpp"

namespace ui {

/* static */ std::vector<DynamicBitmap<16, 16>> ExternalItemsMenuLoader::bitmaps;

// iterates over all ppma-s, and if it is runnable on the current system, it'll call the callback, and pass info.
/* static */ void ExternalItemsMenuLoader::load_all_external_items_callback(std::function<void(AppInfoConsole&)> callback) {
    if (!callback) return;

    if (sd_card::status() != sd_card::Status::Mounted)
        return;

    for (const auto& entry : std::filesystem::directory_iterator(apps_dir, u"*.ppma")) {
        auto filePath = apps_dir / entry.path();
        File app;

        auto openError = app.open(filePath);
        if (openError)
            continue;

        application_information_t application_information = {};

        auto readResult = app.read(&application_information, sizeof(application_information_t));
        if (!readResult)
            continue;

        if (application_information.header_version != CURRENT_HEADER_VERSION)
            continue;

        bool versionMatches = VERSION_MD5 == application_information.app_version;
        if (!versionMatches) continue;
        // here the app is startable and good.
        std::string appshortname = filePath.filename().string();
        if (appshortname.size() >= 5 && appshortname.substr(appshortname.size() - 5) == ".ppma") {
            // Remove the ".ppma" suffix
            appshortname = appshortname.substr(0, appshortname.size() - 5);
        }
        AppInfoConsole info{
            .appCallName = appshortname.c_str(),
            .appFriendlyName = reinterpret_cast<char*>(&application_information.app_name[0]),
            .appLocation = application_information.menu_location};
        callback(info);
    }

    for (const auto& entry : std::filesystem::directory_iterator(apps_dir, u"*.ppmp")) {
        auto filePath = apps_dir / entry.path();
        File app;

        auto openError = app.open(filePath);
        if (openError)
            continue;

        standalone_application_information_t application_information = {};

        auto readResult = app.read(&application_information, sizeof(standalone_application_information_t));
        if (!readResult)
            continue;

        if (application_information.header_version < CURRENT_STANDALONE_APPLICATION_API_VERSION)
            continue;

        // here the app is startable and good.
        std::string appshortname = filePath.filename().string();
        if (appshortname.size() >= 5 && appshortname.substr(appshortname.size() - 5) == ".ppmp") {
            // Remove the ".ppmp" suffix
            appshortname = appshortname.substr(0, appshortname.size() - 5);
        }
        AppInfoConsole info{
            .appCallName = appshortname.c_str(),
            .appFriendlyName = reinterpret_cast<char*>(&application_information.app_name[0]),
            .appLocation = application_information.menu_location};

        callback(info);
    }
}

/* static */ std::vector<GridItem> ExternalItemsMenuLoader::load_external_items(app_location_t app_location, NavigationView& nav) {
    bitmaps.clear();

    std::vector<GridItem> external_apps;

    if (sd_card::status() != sd_card::Status::Mounted)
        return external_apps;

    for (const auto& entry : std::filesystem::directory_iterator(apps_dir, u"*.ppma")) {
        auto filePath = apps_dir / entry.path();
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

            auto dyn_bmp = DynamicBitmap<16, 16>{application_information.bitmap_data};
            gridItem.bitmap = dyn_bmp.bitmap();
            bitmaps.push_back(std::move(dyn_bmp));

            gridItem.on_select = [&nav, app_location, filePath]() {
                if (!run_external_app(nav, filePath)) {
                    nav.display_modal("Error", "The .ppma file in your " + apps_dir.string() + "\nfolder can't be read. Please\nupdate your SD Card content.");
                }
            };
        } else {
            gridItem.color = Theme::getInstance()->fg_light->foreground;

            gridItem.bitmap = &bitmap_sd_card_error;

            gridItem.on_select = [&nav]() {
                nav.display_modal("Error", "The .ppma file in your " + apps_dir.string() + "\nfolder is outdated. Please\nupdate your SD Card content.");
            };
        }

        external_apps.push_back(gridItem);
    }

    for (const auto& entry : std::filesystem::directory_iterator(apps_dir, u"*.ppmp")) {
        auto filePath = apps_dir / entry.path();
        File app;

        auto openError = app.open(filePath);
        if (openError)
            continue;

        standalone_application_information_t application_information = {};

        auto readResult = app.read(&application_information, sizeof(standalone_application_information_t));
        if (!readResult)
            continue;

        if (application_information.menu_location != app_location)
            continue;

        if (application_information.header_version > CURRENT_STANDALONE_APPLICATION_API_VERSION)
            continue;

        GridItem gridItem = {};
        gridItem.text = reinterpret_cast<char*>(&application_information.app_name[0]);

        gridItem.color = Color((uint16_t)application_information.icon_color);

        auto dyn_bmp = DynamicBitmap<16, 16>{application_information.bitmap_data};
        gridItem.bitmap = dyn_bmp.bitmap();
        bitmaps.push_back(std::move(dyn_bmp));

        gridItem.on_select = [&nav, app_location, filePath]() {
            if (!run_standalone_app(nav, filePath)) {
                nav.display_modal("Error", "The .ppmp file in your " + apps_dir.string() + "\nfolder can't be read. Please\nupdate your SD Card content.");
            }
        };

        external_apps.push_back(gridItem);
    }

    return external_apps;
}

/* static */ bool ExternalItemsMenuLoader::run_external_app(ui::NavigationView& nav, std::filesystem::path filePath) {
    File app;
    uint32_t checksum{0};

    auto openError = app.open(filePath);
    if (openError)
        return false;

    application_information_t application_information = {};

    auto readResult = app.read(&application_information, sizeof(application_information_t));
    if (!readResult)
        return false;

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
                return false;

            checksum += simple_checksum((uint32_t)&application_information.memory_location[file_read_index], readResult.value());

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
                return false;

            checksum += simple_checksum((uint32_t)target_memory, readResult.value());

            if (readResult.value() != bytes_to_read)
                break;
        }
    } else {
        // copy application image
        for (size_t file_read_index = 0; file_read_index < 80 * std::filesystem::max_file_block_size; file_read_index += std::filesystem::max_file_block_size) {
            auto bytes_to_read = std::filesystem::max_file_block_size;

            readResult = app.read(&application_information.memory_location[file_read_index], bytes_to_read);
            if (!readResult)
                return false;

            checksum += simple_checksum((uint32_t)&application_information.memory_location[file_read_index], readResult.value());

            if (readResult.value() < std::filesystem::max_file_block_size)
                break;
        }
    }

    if (checksum != EXT_APP_EXPECTED_CHECKSUM)
        return false;

    application_information.externalAppEntry(nav);
    return true;
}

/* static */ bool ExternalItemsMenuLoader::run_standalone_app(ui::NavigationView& nav, std::filesystem::path filePath) {
    File app;

    auto openError = app.open(filePath);
    if (openError)
        return false;

    auto app_image = std::make_unique<uint8_t[]>(app.size());

    // read file in 512 byte chunks
    for (size_t file_read_index = 0; file_read_index < app.size(); file_read_index += std::filesystem::max_file_block_size) {
        auto bytes_to_read = std::filesystem::max_file_block_size;
        if (file_read_index + std::filesystem::max_file_block_size > app.size())
            bytes_to_read = app.size() - file_read_index;

        auto readResult = app.read(&app_image[file_read_index], bytes_to_read);
        if (!readResult)
            return false;

        if (readResult.value() < std::filesystem::max_file_block_size)
            break;
    }

    for (size_t file_read_index = 0; file_read_index < app.size() / 4; file_read_index++) {
        uint32_t* ptr = reinterpret_cast<uint32_t*>(&app_image[file_read_index * 4]);

        if (*ptr >= 0xADB10000 && *ptr < (0xADB10000 + 64 * 1024)) {
            *ptr = *ptr - 0xADB10000 + (uint32_t)app_image.get();
        }
    }

    nav.push<StandaloneView>(std::move(app_image));
    return true;
}

}  // namespace ui
