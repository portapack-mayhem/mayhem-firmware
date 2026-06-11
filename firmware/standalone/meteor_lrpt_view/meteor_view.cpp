/*

 * Copyright (C) 2026

 *

 * CADU .C8 → MSU-MR JPEG → BMP (Meteor LRPT View standalone).

 */

#include "ff.h"



#include "meteor_view.hpp"

#include "meteor_standalone_ui.hpp"

#include "standalone_app.hpp"

#include "jpeg_decode.hpp"

#include "meteor_lrpt_msumr.hpp"

#include "msumr_demux.hpp"



#include <array>

#include <cstdio>

#include <cstring>

#include <new>



namespace {

const standalone_application_api_t* api{nullptr};

uint16_t screen_w{240};

uint16_t screen_h{320};

char status_line[64]{};

meteor_lrpt_g4::MsumrDemux* demux{nullptr};

static std::array<uint8_t, meteor_lrpt::kMeteorCaduRecBytes> g_cadu{};

static FIL s_fil{};
static FILINFO s_fno{};
static DIR s_dir{};
static TCHAR s_path[32]{};

void set_status_ascii(const char* msg) {

    size_t i = 0;

    for (; msg[i] && i + 1 < sizeof(status_line); i++)

        status_line[i] = msg[i];

    status_line[i] = '\0';

}



void draw_ui() {

    meteor_standalone_ui::draw_app_ui(

        screen_w,

        screen_h,

        status_line,

        "In:  /LRPT/dec*.C8",

        "Out: /LRPT/ms*.bmp",

        "SELECT: render BMP");

}



void jpeg_handler(void*, const uint16_t apid, const uint8_t* jpeg, const size_t len) {

    if (!jpeg || len < 4)

        return;

    std::filesystem::path bmp_path{u"/LRPT/ms"};

    bmp_path += std::to_wstring(apid);

    bmp_path += L".bmp";

    uint32_t drops = 0;

    uint8_t jr = 0;

    if (meteor_lrpt_g4::decode_jpeg_to_new_bmp_file(jpeg, len, bmp_path, drops, jr, nullptr)) {

        char tmp[48];

        std::snprintf(tmp, sizeof(tmp), "APID %u -> BMP OK", (unsigned)apid);

        set_status_ascii(tmp);

    } else {

        char tmp[48];

        std::snprintf(tmp, sizeof(tmp), "APID %u decode fail", (unsigned)apid);

        set_status_ascii(tmp);

    }

}



bool process_cadu_file(const TCHAR* path) {

    set_status_ascii("Rendering...");

    draw_ui();

    api->set_dirty();



    if (!demux) {

        void* mem = api->malloc(sizeof(meteor_lrpt_g4::MsumrDemux));

        if (!mem) {

            set_status_ascii("OOM demux");

            return false;

        }

        demux = new (mem) meteor_lrpt_g4::MsumrDemux();

    }

    demux->reset();



    if (f_open(&s_fil, path, FA_READ) != FR_OK)

        return false;



    uint32_t recs = 0;

    uint32_t drops = 0;



    while (true) {

        UINT br = 0;

        if (f_read(&s_fil, g_cadu.data(), static_cast<UINT>(g_cadu.size()), &br) != FR_OK || br == 0)

            break;

        if (br < g_cadu.size())

            break;

        recs++;

        demux->feed_cadu_1020(g_cadu.data(), jpeg_handler, nullptr, drops);

    }

    f_close(&s_fil);

    char tmp[48];

    std::snprintf(tmp, sizeof(tmp), "CADU recs:%u", (unsigned)recs);

    set_status_ascii(tmp);

    return recs > 0;

}



bool view_first_cadu() {

    if (f_findfirst(&s_dir, &s_fno, (const TCHAR*)u"/LRPT", (const TCHAR*)u"dec*.C8") != FR_OK &&

        f_findfirst(&s_dir, &s_fno, (const TCHAR*)u"/LRPT", (const TCHAR*)u"*.C8") != FR_OK) {

        set_status_ascii("No CADU file");

        return false;

    }

    do {

        if (!(s_fno.fattrib & AM_DIR)) {

            size_t o = 0;

            for (const TCHAR* p = (const TCHAR*)u"/LRPT/"; *p && o + 1 < sizeof(s_path) / sizeof(s_path[0]); ++p)

                s_path[o++] = *p;

            for (const TCHAR* p = s_fno.fname; *p && o + 1 < sizeof(s_path) / sizeof(s_path[0]); ++p)

                s_path[o++] = *p;

            s_path[o] = 0;

            f_closedir(&s_dir);

            return process_cadu_file(s_path);

        }

    } while (f_findnext(&s_dir, &s_fno) == FR_OK);

    f_closedir(&s_dir);

    return false;

}



}  // namespace



extern const standalone_application_api_t* _api;



void meteor_view_initialize(const standalone_application_api_t& a) {

    _api = &a;

    api = &a;

    meteor_standalone_ui::bind(a);

    screen_w = *(api->screen_width);

    screen_h = *(api->screen_height);

    set_status_ascii("MeteorVx ready");

    draw_ui();

}



void meteor_view_on_event(const uint32_t&) {}



void meteor_view_shutdown() {

    if (demux) {

        demux->~MsumrDemux();

        api->free(demux);

        demux = nullptr;

    }

}



void meteor_view_paint() {

    draw_ui();

}



bool meteor_view_on_key(uint8_t key) {

    if (meteor_standalone_ui::key_exit_to_menu(key))

        return true;

    if (key != meteor_standalone_ui::kKeySelect)

        return false;

    (void)view_first_cadu();

    draw_ui();

    api->set_dirty();

    return true;

}


