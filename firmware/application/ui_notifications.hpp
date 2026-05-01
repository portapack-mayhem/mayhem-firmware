#pragma once

#include <vector>
#include <string>
#include <memory>
#include "ui.hpp"
#include "ui_widget.hpp"
#include "signal.hpp"
#include "rtc_time.hpp"
#include "event_m0.hpp"

namespace ui {

class NotificationView;
class NavigationView;

enum notification_icon_t : uint8_t {
    NOTIF_ICON_NONE = 0,
    NOTIF_ICON_MESSAGE
};

class NotificationEntry {
   public:
    std::string source_app{};
    std::string title{};
    std::string message{};
    notification_icon_t icon = NOTIF_ICON_NONE;
    uint16_t timeout = 10000;
    uint16_t id = 0;

    NotificationEntry() = default;
    NotificationEntry(std::string source_app, std::string title, std::string message, notification_icon_t icon = NOTIF_ICON_NONE, uint16_t timeout = 10000, uint16_t id = 0)
        : source_app(std::move(source_app)), title(std::move(title)), message(std::move(message)), icon(icon), timeout(timeout), id(id) {}

    static NotificationEntry build(std::string source_app, std::string title, std::string message, notification_icon_t icon = NOTIF_ICON_NONE, uint16_t timeout = 10000) {
        return NotificationEntry{std::move(source_app), std::move(title), std::move(message), icon, timeout, 0};
    }

    bool increase_time(uint16_t delta) {
        current_time += delta;
        return (current_time >= timeout);
    }

   private:
    uint16_t current_time = 0;
};

class NotificationEntryView : public View {
   public:
    NotificationEntryView(NotificationEntry entry, NotificationView* notifhandler);
    NotificationEntryView(const NotificationEntryView&) = delete;
    NotificationEntryView& operator=(const NotificationEntryView&) = delete;

    ui::Rectangle background{{1, 1, UI_POS_MAXWIDTH - 2, UI_POS_HEIGHT(3) - 2}, Theme::getInstance()->bg_darkest->background};
    ui::Rectangle border{{0, 0, UI_POS_MAXWIDTH, UI_POS_HEIGHT(3) + 1}, Theme::getInstance()->bg_light->background};
    ui::Text title_text{{UI_POS_X(0) + 1, UI_POS_Y(0) + 1, UI_POS_WIDTH_REMAINING(4) - 3, UI_POS_HEIGHT(1)}, ""};
    ui::Text message_text{{UI_POS_X(1), UI_POS_Y(1), UI_POS_WIDTH_REMAINING(5) - 1, UI_POS_HEIGHT(2)}, ""};
    ui::Image icon_image{};  // 16*16 bitmap
    ui::Button close_button{{UI_POS_X_RIGHT(4) - 2, UI_POS_Y(0) + 1, UI_POS_WIDTH(4), UI_POS_HEIGHT(2)}, "X"};

    uint16_t id() const { return entry_.id; }
    NotificationEntry* entry() { return &entry_; }

   private:
    NotificationEntry entry_;
    NotificationView* notifhandler_;
};

class NotificationView : public View {
   public:
    NotificationView(NavigationView& nav);
    ~NotificationView();

    void add_notification(NotificationEntry entry);
    void remove_notification(uint16_t id);
    void open_notification(std::string app_name);

   private:
    void on_tick_second();
    void rearrange_notifications();

    NavigationView& nav_;

    // Changed to unique_ptr to handle non-copyable Views safely
    std::vector<std::unique_ptr<NotificationEntryView>> notification_views_{};

    SignalToken signal_token_tick_second{};

    uint16_t curr_not_id = 0;
    constexpr static uint8_t max_notifications = 4;

    MessageHandlerRegistration message_handler_notifs{
        Message::ID::NotificationData, [this](Message* const p) {
            const auto message = static_cast<const NotificationDataMessage*>(p);
            NotificationEntry entry = NotificationEntry::build(message->source_app, message->title, message->message, static_cast<notification_icon_t>(message->icon), message->timeout);
            this->add_notification(entry);
        }};
};

}  // namespace ui