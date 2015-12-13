/*
 * Copyright (C) 2015 Jared Boone, ShareBrained Technology, Inc.
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

#ifndef __UI_DEBUG_H__
#define __UI_DEBUG_H__

#include "ui.hpp"
#include "ui_widget.hpp"
#include "ui_painter.hpp"
#include "ui_menu.hpp"
#include "ui_navigation.hpp"

#include "rffc507x.hpp"
#include "max2837.hpp"
#include "portapack.hpp"

namespace ui {

class DebugMemoryView : public View {
public:
	DebugMemoryView(NavigationView& nav);

	void focus() override;

private:
	Text text_title {
		{ 96, 96, 48, 16 },
		"Memory",
	};

	Text text_label_m0_free {
		{ 0, 128, 104, 16 },
		"M0 Free Bytes",
	};

	Text text_label_m0_free_value {
		{ 200, 128, 40, 16 },
	};

	Text text_label_m0_heap_fragmented_free {
		{ 0, 144, 184, 16 },
		"M0 Heap Fragmented Free",
	};

	Text text_label_m0_heap_fragmented_free_value {
		{ 200, 144, 40, 16 },
	};

	Text text_label_m0_heap_fragments {
		{ 0, 160, 136, 16 },
		"M0 Heap Fragments",
	};

	Text text_label_m0_heap_fragments_value {
		{ 200, 160, 40, 16 },
	};

	Button button_done {
		{ 72, 192, 96, 24 },
		"Done"
	};
};

class DebugRFFC5072RegistersWidget : public Widget {
public:
	constexpr DebugRFFC5072RegistersWidget(
		Rect parent_rect
	) : Widget { parent_rect }
	{
	}

	void update();

	void paint(Painter& painter) override;

	static constexpr const char* name = "RFFC5072";

private:
	static constexpr size_t registers_count { 31 };

	static constexpr size_t legend_length { 2 };
	static constexpr Dim legend_width { legend_length * 8 };

	static constexpr size_t value_length { 4 };
	static constexpr Dim value_width { value_length * 8 };

	static constexpr size_t registers_per_row { 4 };
	static constexpr size_t registers_row_length {
		(registers_per_row * (value_length + 1)) - 1
	};
	static constexpr Dim registers_row_width {
		registers_row_length * 8
	};

	static constexpr size_t rows {
		registers_count / registers_per_row
	};
	static constexpr Dim row_height { 16 };

	void draw_legend(Painter& painter);
	void draw_values(Painter& painter, const rffc507x::RegisterMap registers);
};

class DebugMAX2837RegistersWidget : public Widget {
public:
	constexpr DebugMAX2837RegistersWidget(
		Rect parent_rect
	) : Widget { parent_rect }
	{
	}

	void update();

	void paint(Painter& painter) override;

	static constexpr const char* const name = "MAX2837";

private:
	static constexpr size_t registers_count = 32;

	static constexpr size_t legend_length = 2;
	static constexpr Dim legend_width = legend_length * 8;

	static constexpr size_t value_length = 3;
	static constexpr Dim value_width = value_length * 8;

	static constexpr size_t registers_per_row = 4;
	static constexpr size_t registers_row_length = (registers_per_row * (value_length + 1)) - 1;
	static constexpr Dim registers_row_width = registers_row_length * 8;

	static constexpr size_t rows = registers_count / registers_per_row;
	static constexpr Dim row_height = 16;

	void draw_legend(Painter& painter);
	void draw_values(Painter& painter, const max2837::RegisterMap registers);
};

class DebugSi5351CRegistersWidget : public Widget {
public:
	constexpr DebugSi5351CRegistersWidget(
		Rect parent_rect
	) : Widget { parent_rect }
	{
	}

	void update();

	void paint(Painter& painter) override;

	static constexpr const char* const name = "Si5351C";

private:
	static constexpr size_t registers_count = 96;

	static constexpr size_t legend_length = 2;
	static constexpr Dim legend_width = legend_length * 8;

	static constexpr size_t value_length = 2;
	static constexpr Dim value_width = value_length * 8;

	static constexpr size_t registers_per_row = 8;
	static constexpr size_t registers_row_length = (registers_per_row * (value_length + 1)) - 1;
	static constexpr Dim registers_row_width = registers_row_length * 8;

	static constexpr size_t rows = registers_count / registers_per_row;
	static constexpr Dim row_height = 16;

	void draw_legend(Painter& painter);
	void draw_values(Painter& painter, si5351::Si5351& device);
};

template<class RegistersWidget>
class RegistersView : public View {
public:
	RegistersView(NavigationView& nav) {
		add_children({ {
			&text_title,
			&widget_registers,
			&button_update,
			&button_done,
		} });

		button_update.on_select = [this](Button&){
			this->widget_registers.update();
		};
		button_done.on_select = [&nav](Button&){ nav.pop(); };
	}

	void focus() {
		button_done.focus();
	}

private:
	Text text_title {
		{ 88, 16, 64, 16 },
		RegistersWidget::name,
	};

	RegistersWidget widget_registers {
		{ 0, 48, 240, 192 }
	};

	Button button_update {
		{ 16, 256, 96, 24 },
		"Update"
	};

	Button button_done {
		{ 128, 256, 96, 24 },
		"Done"
	};
};

using DebugRFFC5072View = RegistersView<DebugRFFC5072RegistersWidget>;
using DebugMAX2837View = RegistersView<DebugMAX2837RegistersWidget>;
using DebugSi5351CView = RegistersView<DebugSi5351CRegistersWidget>;

class DebugMenuView : public MenuView {
public:
	DebugMenuView(NavigationView& nav);
};

} /* namespace ui */

#endif/*__UI_DEBUG_H__*/
