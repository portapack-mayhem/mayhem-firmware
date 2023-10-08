/*
 * Copyright (C) 2023 Bernd Herzog
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

#include "ui_calculator.hpp"

using namespace ui;

namespace ui::external_app::calculator {

uint8_t current_key = 255;
char display_string[10];
uint8_t fgm = 0;

// interface to external code
unsigned int mp;

char CHARMAP[] = {
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    'A',
    'B',
    'C',
    'D',
    'E',
    'F',
    'H',
    'I',
    'L',
    'M',
    'N',
    'O',
    'P',
    'R',
    'S',
    'T',
    'U',
    'V',
    'c',
    's',
    't',
    ' ',
    '.',
    '*',
    '+',
    '-',
    '/',
    '!',
    '<',
    '=',
    '>',
    '^',
    'f',
    'n',
    'p',
    's',
    'm',
    'w',

};

void printcat(uint8_t c, uint8_t x) {  // Print char c at position x
    display_string[x] = CHARMAP[c];
}
uint8_t getkey(void) {  // Read keypad
    return current_key;
}

#include "ivt.hpp"

void step() {
    loop();
}

void CalculatorView::focus() {
    button_F.focus();
}

CalculatorView::CalculatorView(NavigationView& nav)
    : nav_{nav} {
    add_children({&button_F,
                  &button_7,
                  &button_8,
                  &button_9,
                  &button_E,
                  &button_4,
                  &button_5,
                  &button_6,
                  &button_N,
                  &button_1,
                  &button_2,
                  &button_3,
                  &button_C,
                  &button_0,
                  &button_P,
                  &button_D,
                  &console});

    button_F.on_select = [&nav, this](Button&) { on_button_press(0); };
    button_7.on_select = [&nav, this](Button&) { on_button_press(1); };
    button_8.on_select = [&nav, this](Button&) { on_button_press(2); };
    button_9.on_select = [&nav, this](Button&) { on_button_press(3); };

    button_E.on_select = [&nav, this](Button&) { on_button_press(4); };
    button_4.on_select = [&nav, this](Button&) { on_button_press(5); };
    button_5.on_select = [&nav, this](Button&) { on_button_press(6); };
    button_6.on_select = [&nav, this](Button&) { on_button_press(7); };

    button_N.on_select = [&nav, this](Button&) { on_button_press(8); };
    button_1.on_select = [&nav, this](Button&) { on_button_press(9); };
    button_2.on_select = [&nav, this](Button&) { on_button_press(10); };
    button_3.on_select = [&nav, this](Button&) { on_button_press(11); };

    button_C.on_select = [&nav, this](Button&) { on_button_press(12); };
    button_0.on_select = [&nav, this](Button&) { on_button_press(13); };
    button_P.on_select = [&nav, this](Button&) { on_button_press(14); };
    button_D.on_select = [&nav, this](Button&) { on_button_press(15); };
}

void CalculatorView::on_button_press(uint8_t button) {
    auto pre_fgm = fgm;
    for (int i = 0; i < 10; i++)
        display_string[i] = ' ';

    current_key = button;
    step();

    do {
        current_key = 255;
        step();
    } while (mp);

    std::string d(&display_string[0], 10);

    if (pre_fgm && button != 0) {
        console.write("      ");
        switch (button) {
            case 1:
                console.write("SUM+");
                break;
            case 2:
                console.write("PRG");
                break;
            case 3:
                console.write("/");
                break;

            case 4:
                console.write("SWAP");
                break;
            case 5:
                console.write("DICT");
                break;
            case 6:
                console.write("USR");
                break;
            case 7:
                console.write("*");
                break;

            case 8:
                console.write("ROT");
                break;
            case 9:
                console.write("RCL");
                break;
            case 10:
                console.write("STO");
                break;
            case 11:
                console.write("-");
                break;

            case 12:
                console.write("CA");
                break;
            case 13:
                console.write("PI");
                break;
            case 14:
                console.write("INT");
                break;
            case 15:
                console.write("+");
                break;
        }
        console.writeln(d);
    } else if (button == 15) {
        console.write("\r");
        console.writeln(d);
    } else {
        console.write("\r");
        console.write(d);
    }

    update_button_labels();
}

void CalculatorView::update_button_labels() {
    if (fgm) {
        button_F.set_text("MENU");
        button_7.set_text("SUM+");
        button_8.set_text("PRG");
        button_9.set_text("/");

        button_E.set_text("SWAP");
        button_4.set_text("DICT");
        button_5.set_text("USR");
        button_6.set_text("*");

        button_N.set_text("ROT");
        button_1.set_text("RCL");
        button_2.set_text("STO");
        button_3.set_text("-");

        button_C.set_text("CA");
        button_0.set_text("PI");
        button_P.set_text("INT");
        button_D.set_text("+");

    } else {
        button_F.set_text("F");
        button_7.set_text("7");
        button_8.set_text("8");
        button_9.set_text("9");

        button_E.set_text("E");
        button_4.set_text("4");
        button_5.set_text("5");
        button_6.set_text("6");

        button_N.set_text("N");
        button_1.set_text("1");
        button_2.set_text("2");
        button_3.set_text("3");

        button_C.set_text("C");
        button_0.set_text("0");
        button_P.set_text(".");
        button_D.set_text("D");
    }
}

}  // namespace ui::external_app::calculator
