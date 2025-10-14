#include "ui_external_items_menu_loader.hpp"

#include "sd_card.hpp"
#include "file_path.hpp"
#include "ui_standalone_view.hpp"

#include "i2cdevmanager.hpp"
#include "i2cdev_ppmod.hpp"

namespace ui {

/* static */ std::vector<DynamicBitmap<16, 16>> ExternalItemsMenuLoader::bitmaps;

// iterates over all possible ext apps-s, and if it is runnable on the current system, it'll call the callback, and pass minimal info. used to print to console, and for autostart setting's app list. where the minimal info is enough
// please keep in sync with load_external_items
/* static */ void ExternalItemsMenuLoader::load_all_external_items_callback(std::function<void(AppInfoConsole&)> callback, bool module_included) {
    if (!callback) return;

    auto dev = (i2cdev::I2cDev_PPmod*)i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDECMDL_PPMOD);

    if (dev && module_included) {
        auto device_info = dev->readDeviceInfo();

        if (device_info.has_value()) {
            for (uint32_t i = 0; i < device_info->application_count; i++) {
                auto appInfo = dev->getStandaloneAppInfo(i);
                if (appInfo.has_value() == false) {
                    continue;
                }

                if (appInfo->header_version > CURRENT_STANDALONE_APPLICATION_API_VERSION)
                    continue;

                AppInfoConsole appInfoConsole = {reinterpret_cast<char*>(&appInfo->app_name[0]), reinterpret_cast<char*>(&appInfo->app_name[0]), appInfo->menu_location};
                callback(appInfoConsole);
            }
        }
    }

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

        if (versionMatches) {
            std::string appshortname = filePath.filename().string();
            if (appshortname.size() >= 5 && appshortname.substr(appshortname.size() - 5) == ".ppma") {
                // Remove the ".ppma" suffix
                appshortname = appshortname.substr(0, appshortname.size() - 5);
            }
            AppInfoConsole appInfoConsole = {appshortname.c_str(), reinterpret_cast<char*>(&application_information.app_name[0]), application_information.menu_location};
            callback(appInfoConsole);
        }
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

        if (application_information.header_version > CURRENT_STANDALONE_APPLICATION_API_VERSION)
            continue;

        std::string appshortname = filePath.filename().string();
        if (appshortname.size() >= 5 && appshortname.substr(appshortname.size() - 5) == ".ppmp") {
            // Remove the ".ppmp" suffix
            appshortname = appshortname.substr(0, appshortname.size() - 5);
        }
        AppInfoConsole appInfoConsole = {appshortname.c_str(), reinterpret_cast<char*>(&application_information.app_name[0]), application_information.menu_location};
        callback(appInfoConsole);
    }
}

/* static */ std::vector<ExternalItemsMenuLoader::GridItemEx> ExternalItemsMenuLoader::load_external_items(app_location_t app_location, NavigationView& nav) {
    bitmaps.clear();

    std::vector<GridItemEx> external_apps;

    auto dev = (i2cdev::I2cDev_PPmod*)i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDECMDL_PPMOD);

    if (dev) {
        auto device_info = dev->readDeviceInfo();

        if (device_info.has_value()) {
            for (uint32_t i = 0; i < device_info->application_count; i++) {
                auto appInfo = dev->getStandaloneAppInfo(i);
                if (appInfo.has_value() == false) {
                    continue;
                }

                if (appInfo->menu_location != app_location) {
                    continue;
                }

                if (appInfo->header_version > CURRENT_STANDALONE_APPLICATION_API_VERSION)
                    continue;

                GridItemEx gridItem = {};
                gridItem.text = reinterpret_cast<char*>(&appInfo->app_name[0]);

                gridItem.color = Color((uint16_t)appInfo->icon_color);

                auto dyn_bmp = DynamicBitmap<16, 16>{appInfo->bitmap_data};
                gridItem.bitmap = dyn_bmp.bitmap();
                bitmaps.push_back(std::move(dyn_bmp));

                gridItem.on_select = [&nav, appInfo, i]() {
                    auto dev2 = (i2cdev::I2cDev_PPmod*)i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDECMDL_PPMOD);
                    dev2->lockDevice();
                    if (dev2) {
                        // auto app_image = reinterpret_cast<uint8_t*>(portapack::memory::map::m4_code.end() - appInfo->binary_size);
                        auto app_image = reinterpret_cast<uint8_t*>(portapack::memory::map::local_sram_0.base());
                        uint8_t errorcnt = 0;
                        for (size_t j = 0; j < appInfo->binary_size; j += 128) {
                            errorcnt = 0;
                            do {
                                auto segment = dev2->downloadStandaloneApp(i, j);
                                if (segment.size() != 128) {
                                    errorcnt++;
                                    if (errorcnt > 5) {
                                        break;
                                    }
                                } else {
                                    std::copy(segment.begin(), segment.end(), app_image + j);
                                    break;
                                }
                            } while (1);  // when ok, break. when errorcnt > 5, break
                            if (errorcnt > 5) {
                                nav.display_modal("Error", "Unable to download app.");
                                dev2->unlockDevice();
                                return;
                            }
                        }

                        if (!run_module_app(nav, app_image, appInfo->binary_size)) {
                            nav.display_modal("Error", "Unable to run downloaded app.");
                        }
                        dev2->unlockDevice();
                    } else
                        nav.display_modal("Error", "Unable to download app.");
                };

                gridItem.desired_position = -1;  // TODO: Where should we put the module's app icon? First? Last? Also configurable?

                external_apps.push_back(gridItem);
            }
        }
    }

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

        GridItemEx gridItem = {};
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

            gridItem.desired_position = application_information.desired_menu_position;
        } else {
            gridItem.color = Theme::getInstance()->fg_light->foreground;

            gridItem.bitmap = &bitmap_sd_card_error;

            gridItem.on_select = [&nav]() {
                nav.display_modal("Error", "The .ppma file in your " + apps_dir.string() + "\nfolder is outdated. Please\nupdate your SD Card content.");
            };

            gridItem.desired_position = application_information.desired_menu_position;
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

        GridItemEx gridItem = {};
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

        gridItem.desired_position = -1;  // No desired position support for standalone apps yet

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

// TODO: implement baseband image support
/* static */ bool ExternalItemsMenuLoader::run_standalone_app(ui::NavigationView& nav, std::filesystem::path filePath) {
    File app;

    auto openError = app.open(filePath);
    if (openError)
        return false;

    auto app_image = reinterpret_cast<uint8_t*>(portapack::memory::map::local_sram_0.base());

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
            *ptr = *ptr - 0xADB10000 + (uint32_t)app_image;
        }
    }

    nav.push<StandaloneView>(app_image);
    return true;
}

// TODO: implement baseband image support
/* static */ bool ExternalItemsMenuLoader::run_module_app(ui::NavigationView& nav, uint8_t* app_image, size_t app_size) {
    for (size_t file_read_index = 0; file_read_index < app_size / 4; file_read_index++) {
        uint32_t* ptr = reinterpret_cast<uint32_t*>(&app_image[file_read_index * 4]);

        if (*ptr >= 0xADB10000 && *ptr < (0xADB10000 + 64 * 1024)) {
            *ptr = *ptr - 0xADB10000 + (uint32_t)app_image;
        }
    }

    nav.push<StandaloneView>(app_image);
    return true;
}

}  // namespace ui
