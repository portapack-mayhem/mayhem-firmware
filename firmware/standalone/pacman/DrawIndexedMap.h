/******************************************************************************/
/*                                                                            */
/*  PACMAN GAME FOR ARDUINO DUE                                               */
/*                                                                            */
/******************************************************************************/
/*  Copyright (c) 2014  Dr. NCX (mirracle.mxx@gmail.com)                      */
/*                                                                            */
/* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL              */
/* WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED              */
/* WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR    */
/* BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES      */
/* OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,     */
/* WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,     */
/* ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS        */
/* SOFTWARE.                                                                  */
/*                                                                            */
/*  MIT license, all text above must be included in any redistribution.       */

// #include "ili9328.h"
#include "standalone_app.hpp"

typedef uint16_t ushort;

#define C16(_rr, _gg, _bb) ((ushort)(((_rr & 0xF8) << 8) | ((_gg & 0xFC) << 3) | ((_bb & 0xF8) >> 3)))

uint16_t _paletteW[] =
    {
        C16(0, 0, 0),
        C16(255, 0, 0),     // 1 red
        C16(222, 151, 81),  // 2 brown
        C16(255, 0, 255),   // 3 pink

        C16(0, 0, 0),
        C16(0, 255, 255),   // 5 cyan
        C16(71, 84, 255),   // 6 mid blue
        C16(255, 184, 81),  // 7 lt brown

        C16(0, 0, 0),
        C16(255, 255, 0),  // 9 yellow
        C16(0, 0, 0),
        C16(33, 33, 255),  // 11 blue

        C16(0, 255, 0),      // 12 green
        C16(71, 84, 174),    // 13 aqua
        C16(255, 184, 174),  // 14 lt pink
        C16(222, 222, 255),  // 15 whiteish
};

void drawIndexedmap(uint8_t* indexmap, int16_t x, uint16_t y) {
    byte i = 0;
    uint16_t color = (uint16_t)_paletteW[indexmap[0]];
    for (byte tmpY = 0; tmpY < 8; tmpY++) {
        byte width = 1;
        for (byte tmpX = 0; tmpX < 8; tmpX++) {
            uint16_t next_color = (uint16_t)_paletteW[indexmap[++i]];
            if ((color != next_color && width >= 1) || tmpX == 7) {
                _api->fill_rectangle(x + tmpX - width + 1, y + tmpY, width, 1, color);
                // painter.draw_hline({x + tmpX - width + 1, y + tmpY}, width, ui::Color(color));

                color = next_color;
                width = 0;
            }
            width++;
        }
    }
}
