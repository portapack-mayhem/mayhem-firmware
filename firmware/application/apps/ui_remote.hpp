/*
 * Copyright (C) 2023 Kyle Reed
 *
 * This file is part of PortaPack.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#include "ui.hpp"
#include "ui_navigation.hpp"

#include "app_settings.hpp"
#include "radio_state.hpp"

#include <string>
#include <vector>

namespace ui {

/* Data model for a remote entry. */
class RemoteEntryModel {
};

/* Data model for a remote. */
class RemoteModel {
   public:
   private:
    std::string name_;
    std::vector<RemoteEntryModel> entries_;
};

/* Settings container for remote. */
struct RemoteSettings {
    std::string remote_path{};
};

class RemoteView : public View {
   public:
    RemoteView(NavigationView& nav);

    std::string title() const override { return "Remote"; };
    void focus() override;

   private:
    NavigationView& nav_;
    RxRadioState radio_state_{};

    // Settings
    RemoteSettings settings_{};
    app_settings::SettingsManager app_settings_{
        "tx_remote"sv,
        app_settings::Mode::TX,
        {
            {"remote_path"sv, &settings_.remote_path},
        }};
};

} /* namespace ui */
