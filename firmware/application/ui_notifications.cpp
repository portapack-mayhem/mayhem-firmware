#include "ui_notifications.hpp"

#include "ui_navigation.hpp"

extern ui::SystemView* system_view_ptr;

namespace ui {

NotificationEntryView::NotificationEntryView(NotificationEntry entry, NotificationView* notifhandler)
    : entry_(std::move(entry)), notifhandler_(notifhandler) {
    add_children({&background, &border, &title_text, &message_text, &close_button});
    border.set_outline(true);
    if (entry_.icon != NOTIF_ICON_NONE) {
        add_child(&icon_image);
        icon_image.set_parent_rect({UI_POS_X(0) + 2, UI_POS_Y(1), UI_POS_WIDTH(2), UI_POS_HEIGHT(1)});
        message_text.set_parent_rect({UI_POS_X(2) + 2, UI_POS_Y(1), UI_POS_WIDTH_REMAINING(6) - 2, UI_POS_HEIGHT(2)});
        if (entry_.icon == NOTIF_ICON_MESSAGE) {
            icon_image.set_bitmap(&bitmap_icon_pocsag);
        }

    } else {
        message_text.set_parent_rect({UI_POS_X(1), UI_POS_Y(1), UI_POS_WIDTH_REMAINING(5) - 1, UI_POS_HEIGHT(2)});
    }
    title_text.set_style(Theme::getInstance()->bg_dark);
    title_text.set(entry_.title);
    message_text.set_style(Theme::getInstance()->bg_darkest);
    message_text.set(entry_.message);

    close_button.on_select = [this](Button&) {
        if (notifhandler_) {
            notifhandler_->remove_notification(entry_.id);
        }
    };

    message_text.on_select = [this](Text&) {
        if (!entry_.source_app.empty() && notifhandler_) {
            notifhandler_->open_notification(entry_.source_app);
            // notifhandler_->remove_notification(entry_.id);
        }
    };
    title_text.on_select = message_text.on_select;
}

NotificationView::NotificationView(NavigationView& nav)
    : nav_(nav) {
    signal_token_tick_second = rtc_time::signal_tick_second += [this]() {
        this->on_tick_second();
    };
}

NotificationView::~NotificationView() {
    rtc_time::signal_tick_second -= signal_token_tick_second;
}

void NotificationView::on_tick_second() {
    bool changed = false;
    for (size_t i = 0; i < notification_views_.size();) {
        if (notification_views_[i]->entry()->increase_time(1000)) {
            notification_views_.erase(notification_views_.begin() + i);
            changed = true;
        } else {
            i++;
        }
    }

    if (changed) {
        rearrange_notifications();
    }
}

void NotificationView::rearrange_notifications() {
    size_t count = children().size();
    while (children().size() > 0) {
        remove_child(children().back());
    }

    const int entry_height = UI_POS_HEIGHT(3) + 1;
    int index = 0;

    for (auto& view_ptr : notification_views_) {
        view_ptr->set_parent_rect({UI_POS_X(0),
                                   (Coord)(UI_POS_Y(0) + (index * entry_height)),
                                   UI_POS_MAXWIDTH,
                                   entry_height});
        add_child(view_ptr.get());
        index++;
    }

    set_parent_rect({UI_POS_X(0), UI_POS_Y(0), UI_POS_MAXWIDTH, (Coord)(notification_views_.size() * entry_height)});
    set_dirty();
    if (count != children().size()) {
        // refresh screen!!!
        system_view_ptr->set_dirty();
    }
}

void NotificationView::add_notification(NotificationEntry entry) {
    if (notification_views_.size() >= max_notifications) {
        notification_views_.erase(notification_views_.begin());
    }
    entry.id = ++curr_not_id;
    notification_views_.push_back(std::make_unique<NotificationEntryView>(std::move(entry), this));

    rearrange_notifications();
}

void NotificationView::remove_notification(uint16_t id) {
    bool found = false;
    size_t found_index = 0;

    for (size_t i = 0; i < notification_views_.size(); i++) {
        if (notification_views_[i]->id() == id) {
            found = true;
            found_index = i;
            break;
        }
    }

    if (found) {
        notification_views_.erase(notification_views_.begin() + found_index);
        rearrange_notifications();
    }
}

void NotificationView::open_notification(std::string app_name) {
    if (app_name.empty()) return;
    nav_.StartAppByName(app_name.c_str());
}

}  // namespace ui