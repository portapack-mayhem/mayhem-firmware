#include "ui_hard_reset.hpp"

#include "portapack.hpp"
#include "file.hpp"
#include "file_path.hpp"
#include "ui_touch_calibration.hpp"

using namespace portapack;
namespace pmem = portapack::persistent_memory;

namespace ui::external_app::hard_reset {
HardResetView::HardResetView(ui::NavigationView& nav)
    : nav_(nav) {
    add_children({&btn_yes,
                  &btn_no,
                  &console_text});

    btn_yes.on_select = [&nav, this](Button&) {
        pmem::cache::defaults();
        clear_settings_folder();
        nav.push<TouchCalibrationView>();
        // nav_.pop();
    };

    btn_no.on_select = [&nav, this](Button&) {
        nav_.pop();
    };
}

HardResetView::~HardResetView() {
}

void HardResetView::on_show() {
    console_text.write("Warning! All app settings and P.Mem will be erased. After   the operation, the touchscreen must be recalibrated.");
}

void HardResetView::focus() {
    btn_no.focus();
}

void HardResetView::delete_all_files_in_directory(const std::filesystem::path& dir_path) {
    for (const auto& entry : std::filesystem::directory_iterator(dir_path, u"*")) {
        if (std::filesystem::is_regular_file(entry.status())) {
            auto full_path = dir_path / entry.path().filename();
            delete_file(full_path);
        }
    }
}

void HardResetView::clear_settings_folder() {
    delete_all_files_in_directory(settings_dir);
}

}  // namespace ui::external_app::hard_reset