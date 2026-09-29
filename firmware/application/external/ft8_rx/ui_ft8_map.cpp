/*
 * Copyright (C) 2026 Dmytro Onyshko
 * Copyright (C) 2026 Khanfar
 * Copyright (C) 2026 gullradriel, Nilorea Studio Inc.
 * Copyright (C) 2026 Mohammad Moghtader (Xmoo26)
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

#include "ui_ft8_map.hpp"

#include "file_path.hpp"
#include "portapack.hpp"
#include "ui_font_fixed_5x8.hpp"

#include <math.h>
#include <stdio.h>
#include <string.h>

using namespace portapack;

namespace ui::external_app::ft8_rx {

static constexpr float deg_to_rad = 0.0174532925f;

/* Spots *********************************************************************/

void FT8Spots::add(const FT8Spot& s) {
    int i = find(s.call);
    if (i < 0 && count == max_spots)
        i = 0;  // full: drop the oldest station
    if (i >= 0) {
        memmove(&spot[i], &spot[i + 1], (count - i - 1) * sizeof(FT8Spot));
        count--;
    }
    spot[count++] = s;
}

int FT8Spots::find(const char* call) const {
    for (size_t i = 0; i < count; i++) {
        if (strcmp(spot[i].call, call) == 0)
            return i;
    }
    return -1;
}

bool locator_to_lat_lon(const char* grid, float& lat, float& lon) {
    if (strlen(grid) != 4 ||
        grid[0] < 'A' || grid[0] > 'R' || grid[1] < 'A' || grid[1] > 'R' ||
        grid[2] < '0' || grid[2] > '9' || grid[3] < '0' || grid[3] > '9')
        return false;
    // A field is 20 x 10 degrees and a square 2 x 1; take the square's center.
    lon = (grid[0] - 'A') * 20.0f + (grid[2] - '0') * 2.0f - 179.0f;
    lat = (grid[1] - 'A') * 10.0f + (grid[3] - '0') - 89.5f;
    return true;
}

bool parse_spot(const char* text, FT8Spot& spot) {
    /* The locator is the last word of a message, and the station that sent it is the
     * word before, or the one before that in a reply that carries an "R". */
    constexpr size_t max_words = 8;
    const char* word[max_words];
    size_t length[max_words];
    size_t words = 0;
    for (const char* p = text; *p && words < max_words;) {
        while (*p == ' ')
            p++;
        if (!*p)
            break;
        word[words] = p;
        while (*p && *p != ' ')
            p++;
        length[words] = p - word[words];
        words++;
    }
    if (words < 2 || length[words - 1] != 4)
        return false;

    char grid[5];
    memcpy(grid, word[words - 1], 4);
    grid[4] = '\0';
    if (strcmp(grid, "RR73") == 0 || !locator_to_lat_lon(grid, spot.lat, spot.lon))
        return false;

    size_t w = words - 2;
    if (length[w] == 1 && word[w][0] == 'R') {
        if (w == 0)
            return false;
        w--;
    }

    // A callsign the decoder only knows by its hash comes out in angle brackets.
    const char* call = word[w];
    size_t len = length[w];
    if (len >= 2 && call[0] == '<' && call[len - 1] == '>') {
        call++;
        len -= 2;
    }
    if (len < 3 || len >= sizeof(spot.call))
        return false;

    bool digit = false, letter = false;
    for (size_t i = 0; i < len; i++) {
        const char c = call[i];
        if (c >= '0' && c <= '9')
            digit = true;
        else if (c >= 'A' && c <= 'Z')
            letter = true;
        else if (c != '/')
            return false;  // "..." for an unknown hash, or not a callsign at all
    }
    if (!digit || !letter)
        return false;

    memcpy(spot.call, call, len);
    spot.call[len] = '\0';
    memcpy(spot.grid, grid, sizeof(spot.grid));
    spot.cq = length[0] == 2 && strncmp(word[0], "CQ", 2) == 0;
    return true;
}

/* Callsign prefix to country, longest prefix wins. Not the full ITU table, the common
 * DX prefixes. One string rather than an array of pointers: "prefix name" per line. */
static const char country_table[] =
    "2E England\n3Z Poland\n4L Georgia\n4S Sri Lanka\n4X Israel\n4Z Israel\n5A Libya\n"
    "5H Tanzania\n5N Nigeria\n5R Madagascar\n5T Mauritania\n5X Uganda\n5Z Kenya\n"
    "6W Senegal\n6Y Jamaica\n7O Yemen\n7P Lesotho\n7X Algeria\n8P Barbados\n8Q Maldives\n"
    "9A Croatia\n9G Ghana\n9H Malta\n9J Zambia\n9K Kuwait\n9L Sierra Leone\n9M Malaysia\n"
    "9N Nepal\n9Q DR Congo\n9V Singapore\n9Y Trinidad\nA United States\nA2 Botswana\n"
    "A4 Oman\nA5 Bhutan\nA6 UAE\nA7 Qatar\nA9 Bahrain\nAP Pakistan\nB China\nBV Taiwan\n"
    "C2 Nauru\nC6 Bahamas\nCE Chile\nCM Cuba\nCN Morocco\nCO Cuba\nCP Bolivia\n"
    "CT Portugal\nCU Azores\nCX Uruguay\nD Germany\nD2 Angola\nDS South Korea\n"
    "DU Philippines\nDZ Philippines\nE5 Cook Isl.\nE7 Bosnia\nEA Spain\nEB Spain\n"
    "EC Spain\nEI Ireland\nEK Armenia\nEL Liberia\nEP Iran\nER Moldova\nES Estonia\n"
    "ET Ethiopia\nEU Belarus\nEW Belarus\nEX Kyrgyzstan\nEY Tajikistan\nEZ Turkmenistan\n"
    "F France\nFK New Caledonia\nFO Fr.Polynesia\nG England\nGD Isle of Man\n"
    "GI N.Ireland\nGM Scotland\nGW Wales\nH4 Solomon Isl.\nHA Hungary\nHB Switzerland\n"
    "HC Ecuador\nHG Hungary\nHI Dominican Rep.\nHK Colombia\nHL South Korea\nHP Panama\n"
    "HR Honduras\nHS Thailand\nHZ Saudi Arabia\nI Italy\nJ Japan\nJ2 Djibouti\n"
    "J3 Grenada\nJ6 St.Lucia\nJ7 Dominica\nJT Mongolia\nJY Jordan\nK United States\n"
    "KH6 Hawaii\nKL7 Alaska\nKP4 Puerto Rico\nLA Norway\nLU Argentina\nLX Luxembourg\n"
    "LY Lithuania\nLZ Bulgaria\nM England\nMD Isle of Man\nMI N.Ireland\nMM Scotland\n"
    "MW Wales\nN United States\nOA Peru\nOD Lebanon\nOE Austria\nOH Finland\n"
    "OK Czech Rep.\nOL Czech Rep.\nOM Slovakia\nON Belgium\nOX Greenland\nOY Faroe Isl.\n"
    "OZ Denmark\nP2 Papua N.Guinea\nP4 Aruba\nPA Netherlands\nPD Netherlands\n"
    "PE Netherlands\nPJ Caribbean NL\nPY Brazil\nPZ Suriname\nR Russia\nS5 Slovenia\n"
    "SM Sweden\nSN Poland\nSO Poland\nSP Poland\nSQ Poland\nST Sudan\nSU Egypt\n"
    "SV Greece\nT7 San Marino\nTA Turkey\nTF Iceland\nTG Guatemala\nTI Costa Rica\n"
    "TJ Cameroon\nTL Centr.Afr.Rep.\nTN Congo\nTR Gabon\nTU Ivory Coast\nTY Benin\n"
    "TZ Mali\nU Russia\nUJ Uzbekistan\nUK Uzbekistan\nUN Kazakhstan\nUR Ukraine\n"
    "US Ukraine\nUT Ukraine\nUX Ukraine\nV2 Antigua\nV3 Belize\nV4 St.Kitts\nV5 Namibia\n"
    "V6 Micronesia\nV7 Marshall Isl.\nV8 Brunei\nVA Canada\nVE Canada\nVK Australia\n"
    "VO Canada\nVP2 Anguilla\nVP5 Turks&Caicos\nVQ9 Diego Garcia\nVU India\nVY Canada\n"
    "W United States\nXE Mexico\nXU Cambodia\nXV Vietnam\nXW Laos\nXX Macao\nXZ Myanmar\n"
    "YB Indonesia\nYI Iraq\nYJ Vanuatu\nYL Latvia\nYN Nicaragua\nYO Romania\n"
    "YS El Salvador\nYU Serbia\nYV Venezuela\nZ2 Zimbabwe\nZ3 N.Macedonia\nZA Albania\n"
    "ZB Gibraltar\nZL New Zealand\nZP Paraguay\nZS South Africa\n";

static const char* country_for_call(const char* call, char (&name)[16]) {
    size_t best = 0;
    const char* found = nullptr;
    for (const char* p = country_table; *p;) {
        const char* space = strchr(p, ' ');
        const size_t len = space - p;
        if (len > best && strncmp(call, p, len) == 0) {
            best = len;
            found = space + 1;
        }
        p = strchr(space, '\n') + 1;
    }
    if (!found)
        return "---";
    const size_t len = strchr(found, '\n') - found;
    memcpy(name, found, len);
    name[len] = '\0';
    return name;
}

/* Great-circle distance and initial bearing from home. */
static uint32_t distance_km(float lat1, float lon1, float lat2, float lon2) {
    const float s_lat = sinf((lat2 - lat1) * deg_to_rad / 2);
    const float s_lon = sinf((lon2 - lon1) * deg_to_rad / 2);
    float a = s_lat * s_lat + cosf(lat1 * deg_to_rad) * cosf(lat2 * deg_to_rad) * s_lon * s_lon;
    // Float rounding can push a just past 1 for near-antipodal points, which would make
    // sqrtf(a) > 1 and asinf() return NaN; clamp so the distance stays valid.
    if (a > 1.0f)
        a = 1.0f;
    return (uint32_t)(2 * 6371.0f * asinf(sqrtf(a)) + 0.5f);
}

static uint32_t bearing_deg(float lat1, float lon1, float lat2, float lon2) {
    const float d_lon = (lon2 - lon1) * deg_to_rad;
    const float y = sinf(d_lon) * cosf(lat2 * deg_to_rad);
    const float x = cosf(lat1 * deg_to_rad) * sinf(lat2 * deg_to_rad) -
                    sinf(lat1 * deg_to_rad) * cosf(lat2 * deg_to_rad) * cosf(d_lon);
    float b = atan2f(y, x) / deg_to_rad;
    if (b < 0)
        b += 360;
    return (uint32_t)(b + 0.5f) % 360;
}

static Color spot_color(const FT8Spot& s) {
    return s.cq ? Color::green() : Color::yellow();
}

/* Cohen-Sutherland clip of a line to a w x h rect, so lines to stations off the map
 * still show their direction without running over the widgets around it. */
static bool clip_line(Point& a, Point& b, int w, int h) {
    int32_t x0 = a.x(), y0 = a.y(), x1 = b.x(), y1 = b.y();
    const auto code = [w, h](int32_t x, int32_t y) {
        return (x < 0 ? 1 : 0) | (x >= w ? 2 : 0) | (y < 0 ? 4 : 0) | (y >= h ? 8 : 0);
    };
    int c0 = code(x0, y0), c1 = code(x1, y1);
    while (c0 | c1) {
        if (c0 & c1)
            return false;
        const int c = c0 ? c0 : c1;
        int32_t x, y;
        if (c & 8) {
            x = x0 + (x1 - x0) * (h - 1 - y0) / (y1 - y0);
            y = h - 1;
        } else if (c & 4) {
            x = x0 + (x1 - x0) * (0 - y0) / (y1 - y0);
            y = 0;
        } else if (c & 2) {
            y = y0 + (y1 - y0) * (w - 1 - x0) / (x1 - x0);
            x = w - 1;
        } else {
            y = y0 + (y1 - y0) * (0 - x0) / (x1 - x0);
            x = 0;
        }
        if (c == c0) {
            x0 = x;
            y0 = y;
            c0 = code(x0, y0);
        } else {
            x1 = x;
            y1 = y;
            c1 = code(x1, y1);
        }
    }
    a = {(int)x0, (int)y0};
    b = {(int)x1, (int)y1};
    return true;
}

/* FT8Map *******************************************************************/

void FT8Map::paint(Painter& painter) {
    GeoMap::paint(painter);
    if (on_paint_overlay)
        on_paint_overlay(painter);
}

bool FT8Map::on_encoder(const EncoderEvent delta) {
    if (delta == 0 || !on_zoom_step)
        return false;
    on_zoom_step(delta > 0 ? 1 : -1);
    return true;
}

/* The map moves once, when the finger lifts: every redraw reads the map file from the
 * SD card, far too slow to follow the finger. */
bool FT8Map::on_touch(const TouchEvent event) {
    switch (event.type) {
        case TouchEvent::Type::Start:
            drag_start_ = event.point;
            return true;
        case TouchEvent::Type::Move:
            return true;
        case TouchEvent::Type::End: {
            const Point d = event.point - drag_start_;
            if (abs(d.x()) + abs(d.y()) > 4)
                pan(d.x(), d.y());
            return true;
        }
        default:
            return false;
    }
}

/* FT8SpotList **************************************************************/

int FT8SpotList::selected() const {
    return selected_call_[0] ? spots_.find(selected_call_) : -1;
}

void FT8SpotList::select(int index) {
    if (index < 0)
        clear_selection();
    else
        strcpy(selected_call_, spots_.spot[index].call);
    set_dirty();
    if (on_change)
        on_change();
}

void FT8SpotList::paint(Painter& painter) {
    const auto r = screen_rect();
    const int n = spots_.count;
    const int visible = rows();
    const int sel = selected();

    // With nothing selected the list follows the newest station.
    const int last_top = n > visible ? n - visible : 0;
    if (sel < 0)
        scroll_ = last_top;
    else if (sel < scroll_)
        scroll_ = sel;
    else if (sel >= scroll_ + visible)
        scroll_ = sel - visible + 1;
    if (scroll_ > last_top)
        scroll_ = last_top;

    painter.fill_rectangle(r, Color::black());

    // 48 columns of the 5 px font across 240 px; the table takes 44.
    char line[49];
    snprintf(line, sizeof(line), "HEARD %-5d GRID COUNTRY        %s", n, have_home ? "      KM BRG" : "");
    painter.draw_string(r.location(), ui::font::fixed_5x8,
                        has_focus() ? Color::black() : Color::light_grey(),
                        has_focus() ? Color::light_grey() : Color::black(), line);

    for (int row = 0; row < visible && scroll_ + row < n; row++) {
        const int i = scroll_ + row;
        const FT8Spot& s = spots_.spot[i];
        char name[16];
        const char* country = country_for_call(s.call, name);
        if (have_home)
            snprintf(line, sizeof(line), "%-11s %-4s %-15.15s %5lukm %3lu", s.call, s.grid, country,
                     (unsigned long)distance_km(home_lat, home_lon, s.lat, s.lon),
                     (unsigned long)bearing_deg(home_lat, home_lon, s.lat, s.lon));
        else
            snprintf(line, sizeof(line), "%-11s %-4s %-15.15s", s.call, s.grid, country);

        const Color bg = (i == sel) ? Color::dark_blue() : Color::black();
        const Point p{r.left(), r.top() + 8 + row * 8};
        painter.fill_rectangle({p, {r.width(), 8}}, bg);
        painter.draw_string(p, ui::font::fixed_5x8, spot_color(s), bg, line);
    }
}

bool FT8SpotList::on_encoder(const EncoderEvent delta) {
    const int n = spots_.count;
    if (n == 0 || delta == 0)
        return false;
    int sel = selected();
    sel = (sel < 0) ? n - 1 : sel + (delta > 0 ? 1 : -1);
    if (sel < 0)
        sel = 0;
    if (sel >= n)
        sel = n - 1;
    select(sel);
    return true;
}

/* A touch on a row selects that station, and a touch on the selected station or the
 * header clears the selection. */
bool FT8SpotList::on_touch(const TouchEvent event) {
    if (event.type != TouchEvent::Type::Start)
        return false;
    const int y = event.point.y() - screen_rect().top() - 8;
    const int i = scroll_ + y / 8;
    if (y < 0 || i >= (int)spots_.count) {
        select(-1);
        return true;
    }
    select(i);
    if (on_open)
        on_open(i);  // tap a row -> open its full-screen detail
    return true;
}

/* Select opens the highlighted station's detail (for the rotary + OK, not just touch). */
bool FT8SpotList::on_key(const KeyEvent key) {
    if (key == KeyEvent::Select && selected() >= 0 && on_open) {
        on_open(selected());
        return true;
    }
    return false;
}

/* FT8SpotDetailView ********************************************************/

FT8SpotDetailView::FT8SpotDetailView(NavigationView& nav, const FT8Spot& spot, bool have_home, float home_lat, float home_lon)
    : nav_{nav}, spot_{spot}, have_home_{have_home}, home_lat_{home_lat}, home_lon_{home_lon} {
    add_children({&button_done});
    button_done.on_select = [this](Button&) { nav_.pop(); };
}

void FT8SpotDetailView::focus() {
    button_done.focus();
}

void FT8SpotDetailView::paint(Painter& painter) {
    // Painter draws in absolute screen coordinates, and screen_rect() starts below
    // the 16 px system status bar. Anchor everything to it so the status bar (and its
    // screenshot button) stays visible instead of being painted over.
    const auto r = screen_rect();
    const auto& s = spot_;
    const auto& font = ui::font::fixed_8x16;
    const Color bg = Color::black();
    const Color label = Theme::getInstance()->fg_light->foreground;

    painter.fill_rectangle(r, bg);

    // Callsign, large, coloured by CQ vs reply (same code as on the map).
    painter.draw_string(r.location() + Point{2 * 8, 1 * 16}, font, s.cq ? Color::green() : Color::yellow(), bg, std::string(s.call));

    int y = 3 * 16;
    const auto row = [&](const char* lbl, const std::string& val) {
        painter.draw_string(r.location() + Point{2 * 8, y}, font, label, bg, lbl);
        painter.draw_string(r.location() + Point{12 * 8, y}, font, Color::white(), bg, val);
        y += 20;
    };

    char buf[16];
    row("Grid", std::string(s.grid));
    char name[16];
    row("Country", country_for_call(s.call, name));
    if (have_home_) {
        snprintf(buf, sizeof(buf), "%lu km", (unsigned long)distance_km(home_lat_, home_lon_, s.lat, s.lon));
        row("Distance", buf);
        snprintf(buf, sizeof(buf), "%lu deg", (unsigned long)bearing_deg(home_lat_, home_lon_, s.lat, s.lon));
        row("Bearing", buf);
    }
    if (s.freq > 0) {
        snprintf(buf, sizeof(buf), "%d Hz", s.freq);
        row("Freq", buf);
    }
    row("Type", s.cq ? "CQ" : "reply");
}

/* FT8MapView ***************************************************************/

FT8MapView::FT8MapView(NavigationView& nav, std::string& qth, FT8Spots& spots, std::function<void()> on_close)
    : nav_{nav}, qth_{qth}, spots_{spots}, on_close_{std::move(on_close)} {
    add_children({&text_qth,
                  &field_qth_field,
                  &field_qth_square,
                  &button_zoom_out,
                  &button_zoom_in,
                  &button_clear,
                  &geomap,
                  &spot_list});

    if (qth_.length() == 4) {
        field_qth_field.set_value(std::string_view{qth_}.substr(0, 2));
        field_qth_square.set_value(std::string_view{qth_}.substr(2, 2));
    }
    const auto on_qth = [this](SymField&) {
        qth_ = field_qth_field.to_string() + field_qth_square.to_string();
        set_home(true);
    };
    field_qth_field.on_change = on_qth;
    field_qth_square.on_change = on_qth;

    button_zoom_out.on_select = [this](Button&) { step_zoom(-1); };
    button_zoom_in.on_select = [this](Button&) { step_zoom(1); };
    button_clear.on_select = [this](Button&) {
        spots_.clear();
        spot_list.clear_selection();
        spots_changed();
    };

    // Focusable so the rotary can zoom it. The view draws its own home marker.
    geomap.set_focusable(true);
    geomap.set_hide_center_marker(true);
    geomap.on_zoom_step = [this](int dir) { step_zoom(dir); };
    geomap.on_paint_overlay = [this](Painter& painter) { draw_overlay(painter); };
    spot_list.on_change = [this]() { geomap.refresh(); };
    spot_list.on_open = [this](int i) {
        if (i >= 0 && i < (int)spots_.count)
            nav_.push<FT8SpotDetailView>(spots_.spot[i], have_home_, home_lat_, home_lon_);
    };

    // Without a map, focus() tells the user and closes the view.
    geomap.set_map_file(adsb_dir / u"world_map_2048.bin");
    if (!geomap.init())
        return;
    geomap.set_zoom(zoom_presets[preset_]);
    geomap.move(0, 20);
    set_home(true);
}

FT8MapView::~FT8MapView() {
    if (on_close_)
        on_close_();
}

void FT8MapView::focus() {
    if (!geomap.map_file_opened())
        nav_.display_modal("No map", "No world_map_2048.bin in\n/" + adsb_dir.string() + "/ directory", ABORT);
    else
        geomap.focus();
}

void FT8MapView::spots_changed() {
    spot_list.set_dirty();
    geomap.refresh();
}

void FT8MapView::set_home(bool recenter) {
    have_home_ = locator_to_lat_lon(qth_.c_str(), home_lat_, home_lon_);
    spot_list.have_home = have_home_;
    spot_list.home_lat = home_lat_;
    spot_list.home_lon = home_lon_;
    spot_list.set_dirty();
    if (have_home_ && recenter)
        geomap.move(home_lon_, home_lat_);
    geomap.refresh();
}

void FT8MapView::step_zoom(int dir) {
    if (dir > 0 && preset_ > 0)
        preset_--;
    else if (dir < 0 && preset_ < zoom_preset_count - 1)
        preset_++;
    geomap.set_zoom(zoom_presets[preset_]);
}

/* Maidenhead field letters along the top and left edges, A-R in 20 degree columns and
 * 10 degree rows. Zoomed in they would only clutter the map. */
void FT8MapView::draw_field_letters(Painter& painter, const Rect& r) {
    char letter[2] = {'A', '\0'};
    for (int i = 0; i < 18; i++) {
        const int x = geomap.geo_to_pixel(0, -170.0f + i * 20).x();
        letter[0] = 'A' + i;
        if (x >= 12 && x < r.width() - 6)
            painter.draw_string({r.left() + x - 2, r.top() + 1}, ui::font::fixed_5x8, Color::white(), Color::black(), letter);
    }
    for (int j = 0; j < 18; j++) {
        const int y = geomap.geo_to_pixel(-85.0f + j * 10, 0).y();
        letter[0] = 'A' + j;
        if (y >= 12 && y < r.height() - 12)
            painter.draw_string({r.left() + 1, r.top() + y - 4}, ui::font::fixed_5x8, Color::white(), Color::black(), letter);
    }
}

void FT8MapView::draw_overlay(Painter& painter) {
    const auto r = geomap.screen_rect();
    const int sel = spot_list.selected();

    if (geomap.zoom() <= 1)
        draw_field_letters(painter, r);

    const Point home = have_home_ ? geomap.geo_to_pixel(home_lat_, home_lon_) : Point{};
    const auto segment = [&](Point a, Point b, Color color) {
        if (clip_line(a, b, r.width(), r.height()))
            display.draw_line(a + r.location(), b + r.location(), color);
    };
    // A straight line from home to the station, taken the short way round. If that way
    // crosses the antimeridian the station's longitude is shifted off the near edge (so
    // the line leaves the map at the correct side, clipped there) and a mirror segment is
    // drawn from the station's real position toward home off the opposite edge, instead of
    // one segment streaking back across the whole map.
    const auto line_to = [&](const FT8Spot& s, Color color) {
        float dlon = s.lon - home_lon_;
        if (dlon > 180.0f)
            dlon -= 360.0f;
        else if (dlon < -180.0f)
            dlon += 360.0f;
        const float near_lon = home_lon_ + dlon;
        segment(home, geomap.geo_to_pixel(s.lat, near_lon), color);
        if (near_lon > 180.0f || near_lon < -180.0f)
            segment(geomap.geo_to_pixel(s.lat, s.lon), geomap.geo_to_pixel(home_lat_, s.lon - dlon), color);
    };
    const auto dot = [&](Point p, Color color) {
        if (p.x() >= 2 && p.x() < r.width() - 2 && p.y() >= 2 && p.y() < r.height() - 2)
            display.fill_rectangle({p + r.location() - Point(2, 2), {5, 5}}, color);
    };

    // Selected station last, so nothing is drawn over it.
    if (have_home_) {
        for (size_t i = 0; i < spots_.count; i++) {
            if ((int)i != sel)
                line_to(spots_.spot[i], spot_color(spots_.spot[i]));
        }
        if (sel >= 0)
            line_to(spots_.spot[sel], Color::white());
    }
    for (size_t i = 0; i < spots_.count; i++)
        dot(geomap.geo_to_pixel(spots_.spot[i].lat, spots_.spot[i].lon), spot_color(spots_.spot[i]));
    if (sel >= 0)
        dot(geomap.geo_to_pixel(spots_.spot[sel].lat, spots_.spot[sel].lon), Color::white());
    if (have_home_)
        dot(home, Color::cyan());
}

}  // namespace ui::external_app::ft8_rx
