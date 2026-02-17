#pragma once

#include "ui_navigation.hpp"

namespace ui {

enum notification_icon_t : uint8_t {
    NOTIF_ICON_NONE = 0,
    NOTIF_ICON_MESSAGE
};

class NotificationEntry {
   public:
    std::string source_app{};  // on click this app will be opened, if empty, no app will be opened
    std::string title{};
    std::string message{};
    notification_icon_t icon = NOTIF_ICON_NONE;
    uint16_t timeout = 5000;  // ms

    static NotificationEntry build(const std::string& source_app, const std::string& title, const std::string& message, notification_icon_t icon = NOTIF_ICON_NONE, uint16_t timeout = 5000) {
        return NotificationEntry{source_app, title, message, icon, timeout};
    }
};

class NotificationEntryView : public View {
   public:
    NotificationEntryView(const NotificationEntry& entry);
    void paint(Painter& painter) override;

   private:
    const NotificationEntry& entry_;
};

class NotificationView : public View {
   public:
    NotificationView(NavigationView& nav);
    void paint(Painter& painter) override;

    void add_notification(NotificationEntry& entry);
};

}  // namespace ui