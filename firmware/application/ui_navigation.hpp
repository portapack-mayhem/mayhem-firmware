/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
 * Copyright (C) 2016 Furrtek
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

#ifndef __UI_NAVIGATION_H__
#define __UI_NAVIGATION_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_focus.hpp"
#include "ui_menu.hpp"
#include "ui_btngrid.hpp"

#include "ui_rssi.hpp"
#include "ui_channel.hpp"
#include "ui_audio.hpp"
#include "ui_sd_card_status_view.hpp"

#include "bitmap.hpp"
#include "ff.h"
#include "diskio.h"
#include "lfsr_random.hpp"
#include "sd_card.hpp"

#include <vector>
#include <utility>

using namespace sd_card;

namespace ui
{

	enum modal_t
	{
		INFO = 0,
		YESNO,
		YESCANCEL,
		ABORT
	};

	class NavigationView : public View
	{
	public:
		std::function<void(const View &)> on_view_changed{};

		NavigationView() = default;

		NavigationView(const NavigationView &) = delete;
		NavigationView(NavigationView &&) = delete;
		NavigationView &operator=(const NavigationView &) = delete;
		NavigationView &operator=(NavigationView &&) = delete;

		bool is_top() const;

		template <class T, class... Args>
		T *push(Args &&...args)
		{
			return reinterpret_cast<T *>(push_view(std::unique_ptr<View>(new T(*this, std::forward<Args>(args)...))));
		}
		template <class T, class... Args>
		T *replace(Args &&...args)
		{
			pop();
			return reinterpret_cast<T *>(push_view(std::unique_ptr<View>(new T(*this, std::forward<Args>(args)...))));
		}

		void push(View *v);
		void replace(View *v);

		void pop();
		void pop_modal();

		void display_modal(const std::string &title, const std::string &message);
		void display_modal(const std::string &title, const std::string &message, const modal_t type, const std::function<void(bool)> on_choice = nullptr);

		void focus() override;

	private:
		std::vector<std::unique_ptr<View>> view_stack{};
		Widget *modal_view{nullptr};

		Widget *view() const;

		void free_view();
		void update_view();
		View *push_view(std::unique_ptr<View> new_view);
	};

	class SystemStatusView : public View
	{
	public:
		std::function<void(void)> on_back{};

		SystemStatusView(NavigationView &nav);

		void set_back_enabled(bool new_value);
		void set_title_image_enabled(bool new_value);
		void set_title(const std::string new_value);

	private:
		static constexpr auto default_title = "";

		NavigationView &nav_;

		Rectangle backdrop{
			{0 * 8, 0 * 16, 240, 16},
			Color::dark_grey()};

		ImageButton button_back{
			{2, 0 * 16, 16, 16},
			&bitmap_icon_previous,
			Color::white(),
			Color::dark_grey()};

		Text title{
			{20, 0, 14 * 8, 1 * 16},
			default_title,
		};

		ImageButton button_title{
			{2, 0, 80, 16},
			&bitmap_titlebar_image,
			Color::white(),
			Color::dark_grey()};

		ImageButton button_speaker{
			{17 * 8, 0, 2 * 8, 1 * 16},
			&bitmap_icon_speaker_mute,
			Color::light_grey(),
			Color::dark_grey()};

		ImageButton button_stealth{
			{19 * 8, 0, 2 * 8, 1 * 16},
			&bitmap_icon_stealth,
			Color::light_grey(),
			Color::dark_grey()};

		/*ImageButton button_textentry {
		{ 170, 0, 2 * 8, 1 * 16 },
		&bitmap_icon_unistroke,
		Color::white(),
		Color::dark_grey()
	};*/

		ImageButton button_camera{
			{21 * 8, 0, 2 * 8, 1 * 16},
			&bitmap_icon_camera,
			Color::white(),
			Color::dark_grey()};

		ImageButton button_sleep{
			{23 * 8, 0, 2 * 8, 1 * 16},
			&bitmap_icon_sleep,
			Color::white(),
			Color::dark_grey()};

		ImageButton button_bias_tee{
			{25 * 8, 0, 12, 1 * 16},
			&bitmap_icon_biast_off,
			Color::light_grey(),
			Color::dark_grey()};

		ImageButton button_clock_status{
			{27 * 8, 0 * 16, 2 * 8, 1 * 16},
			&bitmap_icon_clk_int,
			Color::light_grey(),
			Color::dark_grey()};

		SDCardStatusView sd_card_status_view{
			{28 * 8, 0 * 16, 2 * 8, 1 * 16}};

		void on_speaker();
		void on_stealth();
		void on_bias_tee();
		//void on_textentry();
		void on_camera();
		void on_title();
		void refresh();
		void on_clk();

		MessageHandlerRegistration message_handler_refresh{
			Message::ID::StatusRefresh,
			[this](const Message *const p)
			{
				(void)p;
				this->refresh();
			}};
	};

	class InformationView : public View
	{
	public:
		InformationView(NavigationView &nav);
		void refresh();

	private:
		static constexpr auto version_string = "v1.4.3";
		NavigationView &nav_;

		Rectangle backdrop{
			{0, 0 * 16, 240, 16},
			{33, 33, 33}};

		Text version{
			{2, 0, 11 * 8, 16},
			version_string};

		LiveDateTime ltime{
			{86, 0, 19 * 8, 16}};
	};

	class BMPView : public View
	{
	public:
		BMPView(NavigationView &nav);
		void paint(Painter &) override;
		void focus() override;

	private:
		Text text_info{
			{4 * 8, 284, 20 * 8, 16},
			"Version " VERSION_STRING};

		Button button_done{
			{240, 0, 1, 1},
			""};
	};

	class ReceiversMenuView : public BtnGridView
	{
	public:
		ReceiversMenuView(NavigationView &nav);
		std::string title() const override { return "Receivers"; };
	};

	class TransmittersMenuView : public BtnGridView
	{
	public:
		TransmittersMenuView(NavigationView &nav);
		std::string title() const override { return "Transmitters"; };
	};

	class UtilitiesMenuView : public BtnGridView
	{
	public:
		UtilitiesMenuView(NavigationView &nav);
		std::string title() const override { return "Utilities"; };
	};

	class SystemMenuView : public BtnGridView
	{
	public:
		SystemMenuView(NavigationView &nav);

	private:
		void hackrf_mode(NavigationView &nav);
	};

	class SystemView : public View
	{
	public:
		SystemView(
			Context &context,
			const Rect parent_rect);

		Context &context() const override;

	private:
		SystemStatusView status_view{navigation_view};
		InformationView info_view{navigation_view};
		NavigationView navigation_view{};
		Context &context_;
	};

	/*class NotImplementedView : public View {
public:
	NotImplementedView(NavigationView& nav);

	void focus() override;

private:
	Text text_title {
		{ 5 * 8, 7 * 16, 19 * 8, 16 },
		"Not Yet Implemented"
	};

	Button button_done {
		{ 10 * 8, 13 * 16, 10 * 8, 24 },
		"Bummer",
	};
};*/

	class ModalMessageView : public View
	{
	public:
		ModalMessageView(
			NavigationView &nav,
			const std::string &title,
			const std::string &message,
			const modal_t type,
			const std::function<void(bool)> on_choice);

		void paint(Painter &painter) override;
		void focus() override;

		std::string title() const override { return title_; };

	private:
		const std::string title_;
		const std::string message_;
		const modal_t type_;
		const std::function<void(bool)> on_choice_;

		Button button_ok{
			{10 * 8, 14 * 16, 10 * 8, 48},
			"OK",
		};

		Button button_yes{
			{5 * 8, 14 * 16, 8 * 8, 48},
			"YES",
		};

		Button button_no{
			{17 * 8, 14 * 16, 8 * 8, 48},
			"NO",
		};
	};

} /* namespace ui */

#endif /*__UI_NAVIGATION_H__*/
