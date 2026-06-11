#include "ui_external_items_menu_loader.hpp"

#include "baseband_api.hpp"
#include "event_m0.hpp"
#include "memory_map.hpp"
#include "meteor_lrpt_m0_storage.hpp"
#include "spi_image.hpp"

#include "lpc43xx_cpp.hpp"
#include "message.hpp"
#include "sd_card.hpp"
#include "file_path.hpp"
#include "ui_standalone_view.hpp"

#include <algorithm>
#include <cstring>

#include <ch.h>

#include "i2cdevmanager.hpp"
#include "i2cdev_ppmod.hpp"

namespace ui {

namespace {

constexpr uint32_t kStandaloneLinkBase = 0xADB10000u;

void relocate_standalone_image(uint8_t* const app_image, const size_t image_bytes) {
    const uint32_t runtime_base = reinterpret_cast<uint32_t>(app_image);
    for (size_t i = 0; i < image_bytes / 4; i++) {
        auto* const ptr = reinterpret_cast<uint32_t*>(&app_image[i * 4]);
        uint32_t v = *ptr;
        if (v >= kStandaloneLinkBase && v < kStandaloneLinkBase + image_bytes)
            v = v - kStandaloneLinkBase + runtime_base;
        else if (v >= runtime_base && v < runtime_base + image_bytes)
            v = v; /* already runtime-relative */
        else
            continue;
        *ptr = v;
    }
}

ExternalAppLoadError g_external_app_last_load_error{ExternalAppLoadError::None};

void set_load_error(const ExternalAppLoadError error) {
    g_external_app_last_load_error = error;
}

template <size_t W, size_t H>
std::unique_ptr<DynamicBitmap<W, H>> allocate_menu_bitmap(const uint8_t* const bitmap_data) {
    using Bmp = DynamicBitmap<W, H>;
    void* const mem = chHeapAlloc(nullptr, sizeof(Bmp));
    if (!mem)
        return nullptr;
    return std::unique_ptr<Bmp>(new (mem) Bmp(bitmap_data));
}

}  // namespace

/* static */ ExternalAppLoadError ExternalItemsMenuLoader::external_app_last_load_error() {
    return g_external_app_last_load_error;
}

/* static */ const char* ExternalItemsMenuLoader::external_app_load_error_message(const ExternalAppLoadError error) {
    switch (error) {
        case ExternalAppLoadError::None:
            return "Unknown error.";
        case ExternalAppLoadError::FileRead:
            return "Could not read the .ppma file.";
        case ExternalAppLoadError::M4TooLarge:
            return "M4 baseband image exceeds 26 KiB.";
        case ExternalAppLoadError::M0TooLarge:
            return "M0 UI section exceeds 10 KiB slot.";
        case ExternalAppLoadError::BasebandSyncTimeout:
            return "M4 baseband did not start in time.";
        case ExternalAppLoadError::ChecksumMismatch:
            return "Checksum mismatch (corrupt .ppma).";
        case ExternalAppLoadError::InvalidLayout:
            return "Invalid .ppma layout or address.";
        default:
            return "Could not load application.";
    }
}

/* static */ std::vector<std::unique_ptr<DynamicBitmap<16, 16>>> ExternalItemsMenuLoader::bitmaps;

// to save ram when entering an app
void ExternalItemsMenuLoader::unload_external_items() {
    bitmaps.clear();
    bitmaps.shrink_to_fit();
}

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

                auto dyn_bmp = allocate_menu_bitmap<16, 16>(appInfo->bitmap_data);
                if (!dyn_bmp)
                    break;
                gridItem.bitmap = dyn_bmp->bitmap();
                bitmaps.push_back(std::move(dyn_bmp));

                gridItem.on_select = [&nav, appInfo, i]() {
                    auto dev2 = (i2cdev::I2cDev_PPmod*)i2cdev::I2CDevManager::get_dev_by_model(I2C_DEVMDL::I2CDECMDL_PPMOD);
                    if (dev2) {
                        dev2->lockDevice();
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

            auto dyn_bmp = allocate_menu_bitmap<16, 16>(application_information.bitmap_data);
            if (!dyn_bmp)
                break;
            gridItem.bitmap = dyn_bmp->bitmap();
            bitmaps.push_back(std::move(dyn_bmp));

            gridItem.on_select = [&nav, app_location, filePath]() {
                if (!run_external_app(nav, filePath)) {
                    const auto err = ExternalItemsMenuLoader::external_app_last_load_error();
                    nav.display_modal("Error", ExternalItemsMenuLoader::external_app_load_error_message(err));
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

        auto dyn_bmp = allocate_menu_bitmap<16, 16>(application_information.bitmap_data);
        if (!dyn_bmp)
            break;
        gridItem.bitmap = dyn_bmp->bitmap();
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
    unload_external_items();
    set_load_error(ExternalAppLoadError::None);

    File app;
    uint32_t checksum{0};

    auto openError = app.open(filePath);
    if (openError) {
        set_load_error(ExternalAppLoadError::FileRead);
        return false;
    }

    application_information_t application_information = {};

    auto readResult = app.read(&application_information, sizeof(application_information_t));
    if (!readResult) {
        set_load_error(ExternalAppLoadError::FileRead);
        return false;
    }

    if (app.seek(0).is_error()) {
        set_load_error(ExternalAppLoadError::FileRead);
        return false;
    }

    if (application_information.m4_app_offset != 0) {
        const auto file_size = app.size();
        constexpr size_t ppma_checksum_trailer = 4;
        if (file_size <= application_information.m4_app_offset + ppma_checksum_trailer) {
            set_load_error(ExternalAppLoadError::InvalidLayout);
            return false;
        }
        const auto m4_payload_bytes = file_size - application_information.m4_app_offset - ppma_checksum_trailer;
        if (m4_payload_bytes > portapack::memory::map::m4_code.size()) {
            set_load_error(ExternalAppLoadError::M4TooLarge);
            return false;
        }

        std::memset(reinterpret_cast<void*>(portapack::memory::map::m4_code.base()), 0, portapack::memory::map::m4_code.size());

        const auto m0_region = portapack::memory::map::m0_external_app_runtime;
        if (application_information.m4_app_offset > m0_region.size()) {
            set_load_error(ExternalAppLoadError::M0TooLarge);
            return false;
        }
        const bool is_meteor_ppma =
            application_information.m4_app_tag == portapack::spi_flash::image_tag_meteor_lrpt_capture ||
            application_information.m4_app_tag == portapack::spi_flash::image_tag_meteor_lrpt_rx;
        if (is_meteor_ppma && application_information.m4_app_offset > ui::external_app::meteor_lrpt_rx::kMeteorM0PackagedCodeMaxBytes) {
            set_load_error(ExternalAppLoadError::M0TooLarge);
            return false;
        }
        /* memory_location is uint8_t* in application_information_t (offset 0), not a byte array. */
        const uint32_t packaged_m0_addr =
            reinterpret_cast<uint32_t>(application_information.memory_location);
        const bool addr_ok = (packaged_m0_addr == 0u) ||
                             (packaged_m0_addr == m0_region.base()) ||
                             (packaged_m0_addr >= 0xADB00000u && packaged_m0_addr < 0xAE080000u);
        if (!addr_ok) {
            set_load_error(ExternalAppLoadError::InvalidLayout);
            return false;
        }
        auto* const m0_dest = reinterpret_cast<uint8_t*>(m0_region.base());

        // M4 first (bank2), then M0 UI in AHB tail — never load M0 UI into local_sram_0 (M4 Bank1).
        if (app.seek(application_information.m4_app_offset).is_error()) {
            set_load_error(ExternalAppLoadError::FileRead);
            return false;
        }
        for (size_t file_read_index = application_information.m4_app_offset;; file_read_index += readResult.value()) {
            size_t bytes_to_read = std::filesystem::max_file_block_size;

            // not aligned
            if ((file_read_index % std::filesystem::max_file_block_size) != 0)
                bytes_to_read = std::filesystem::max_file_block_size - (file_read_index % std::filesystem::max_file_block_size);

            if (bytes_to_read == 0)
                break;

            const size_t m4_off = file_read_index - application_information.m4_app_offset;
            if (m4_off >= m4_payload_bytes)
                break;
            bytes_to_read = std::min(bytes_to_read, static_cast<size_t>(m4_payload_bytes - m4_off));

            auto target_memory = reinterpret_cast<void*>(portapack::memory::map::m4_code.base() + m4_off);

            readResult = app.read(target_memory, bytes_to_read);
            if (!readResult) {
                set_load_error(ExternalAppLoadError::FileRead);
                return false;
            }

            checksum += simple_checksum((uint32_t)target_memory, readResult.value());

            if (readResult.value() != bytes_to_read)
                break;
        }

        if (app.seek(0).is_error()) {
            set_load_error(ExternalAppLoadError::FileRead);
            return false;
        }
        std::memset(m0_dest, 0, m0_region.size());

        // M0 section: load into fixed SRAM slot (not heap; ignore header memory_location).
        for (size_t file_read_index = 0; file_read_index < application_information.m4_app_offset;) {
            const auto bytes_to_read = std::min(
                static_cast<size_t>(std::filesystem::max_file_block_size),
                static_cast<size_t>(application_information.m4_app_offset - file_read_index));

            readResult = app.read(m0_dest + file_read_index, bytes_to_read);
            if (!readResult || readResult.value() == 0) {
                set_load_error(ExternalAppLoadError::FileRead);
                return false;
            }

            checksum += simple_checksum((uint32_t)(m0_dest + file_read_index), readResult.value());
            file_read_index += readResult.value();
        }

        uint32_t ppma_checksum_word{0};
        if (app.seek(file_size - ppma_checksum_trailer).is_error()) {
            set_load_error(ExternalAppLoadError::FileRead);
            return false;
        }
        readResult = app.read(&ppma_checksum_word, ppma_checksum_trailer);
        if (!readResult) {
            set_load_error(ExternalAppLoadError::FileRead);
            return false;
        }
        checksum += simple_checksum((uint32_t)&ppma_checksum_word, readResult.value());
    } else {
        // copy application image
        for (size_t file_read_index = 0; file_read_index < 80 * std::filesystem::max_file_block_size; file_read_index += std::filesystem::max_file_block_size) {
            auto bytes_to_read = std::filesystem::max_file_block_size;

            readResult = app.read(&application_information.memory_location[file_read_index], bytes_to_read);
            if (!readResult) {
                set_load_error(ExternalAppLoadError::FileRead);
                return false;
            }

            checksum += simple_checksum((uint32_t)&application_information.memory_location[file_read_index], readResult.value());

            if (readResult.value() < std::filesystem::max_file_block_size)
                break;
        }
    }

    if (checksum != EXT_APP_EXPECTED_CHECKSUM) {
        set_load_error(ExternalAppLoadError::ChecksumMismatch);
        return false;
    }

    nav.pop();
    nav.set_last_menu_went_deeper(true);
    if (application_information.m4_app_offset != 0) {
        MessageHandlerRegistration::unregister_id(Message::ID::MeteorLrptRxStatusData);
        MessageHandlerRegistration::unregister_id(Message::ID::MeteorLrptRxPreviewLine);
        auto* const loaded = reinterpret_cast<application_information_t*>(portapack::memory::map::m0_external_app_runtime.base());
        creg::m4txevent::disable();
        loaded->externalAppEntry(nav);
        /* M4 events stay masked until capture view focus() enables them after handler setup. */
    } else {
        application_information.externalAppEntry(nav);
    }
    return true;
}

// TODO: implement baseband image support
/* static */ bool ExternalItemsMenuLoader::run_standalone_app(ui::NavigationView& nav, std::filesystem::path filePath) {
    File app;

    auto openError = app.open(filePath);
    if (openError)
        return false;

    constexpr size_t kStandaloneAppMaxBytes = 96u * 1024u;
    if (app.size() > kStandaloneAppMaxBytes)
        return false;

    auto app_image = reinterpret_cast<uint8_t*>(portapack::memory::map::local_sram_0.base());

    /* .ppmp is raw loadable image; BSS past file tail must be zero (decode uses ~70 KiB RAM). */
    std::memset(app_image, 0, kStandaloneAppMaxBytes);

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

    relocate_standalone_image(app_image, kStandaloneAppMaxBytes);

    nav.push<StandaloneView>(app_image);
    return true;
}

// TODO: implement baseband image support
/* static */ bool ExternalItemsMenuLoader::run_module_app(ui::NavigationView& nav, uint8_t* app_image, size_t app_size) {
    constexpr size_t kModuleAppMaxBytes = 96u * 1024u;
    if (app_size < kModuleAppMaxBytes)
        std::memset(app_image + app_size, 0, kModuleAppMaxBytes - app_size);

    relocate_standalone_image(app_image, kModuleAppMaxBytes);

    nav.push<StandaloneView>(app_image);
    return true;
}

}  // namespace ui
