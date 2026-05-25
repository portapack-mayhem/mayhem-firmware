/*
 * Copyright (C) 2026
 *
 * Meteor LRPT View — standalone Receive app.
 */
#include "standalone_app.hpp"
#include "meteor_view.hpp"

const standalone_application_api_t* _api;

extern "C" {
__attribute__((section(".standalone_application_information"), used)) standalone_application_information_t
    _standalone_application_information = {
        2,
        "MeteorVx",
        {0x00, 0x1C, 0x3E, 0x7F, 0x7F, 0x3E, 0x1C, 0x08, 0x08, 0x1C, 0x3E, 0x7F, 0x7F, 0x3E, 0x1C, 0x00,
         0x00, 0x18, 0x3C, 0x7E, 0xFF, 0x7E, 0x3C, 0x18, 0x18, 0x3C, 0x7E, 0xFF, 0x7E, 0x3C, 0x18, 0x00},
        0x07E0,
        app_location_t::RX,
        meteor_view_initialize,
        meteor_view_on_event,
        meteor_view_shutdown,
        meteor_view_paint,
        nullptr,
        nullptr,
        meteor_view_on_key,
        nullptr,
        nullptr};
}

extern "C" void abort() {
    while (true) {
    }
}

extern "C" void* malloc(size_t size) {
    return _api->malloc(size);
}
extern "C" void* calloc(size_t num, size_t size) {
    return _api->calloc(num, size);
}
extern "C" void* realloc(void* p, size_t size) {
    return _api->realloc(p, size);
}
extern "C" void free(void* p) {
    _api->free(p);
}
extern "C" void* __wrap__malloc_r(size_t size) {
    return _api->malloc(size);
}
extern "C" void __wrap__free_r(void* p) {
    _api->free(p);
}

#include "ff.h"
extern "C" FRESULT f_open(FIL* fp, const TCHAR* path, BYTE mode) {
    return _api->f_open(fp, path, mode);
}
extern "C" FRESULT f_close(FIL* fp) {
    return _api->f_close(fp);
}
extern "C" FRESULT f_read(FIL* fp, void* buff, UINT btr, UINT* br) {
    return _api->f_read(fp, buff, btr, br);
}
extern "C" FRESULT f_write(FIL* fp, const void* buff, UINT btw, UINT* bw) {
    return _api->f_write(fp, buff, btw, bw);
}
extern "C" FRESULT f_lseek(FIL* fp, FSIZE_t ofs) {
    return _api->f_lseek(fp, ofs);
}
extern "C" FRESULT f_sync(FIL* fp) {
    return _api->f_sync(fp);
}
extern "C" FRESULT f_unlink(const TCHAR* path) {
    return _api->f_unlink(path);
}
extern "C" FRESULT f_stat(const TCHAR* path, FILINFO* fno) {
    return _api->f_stat(path, fno);
}
extern "C" FRESULT f_opendir(DIR* dp, const TCHAR* path) {
    return _api->f_opendir(dp, path);
}
extern "C" FRESULT f_closedir(DIR* dp) {
    return _api->f_closedir(dp);
}
extern "C" FRESULT f_readdir(DIR* dp, FILINFO* fno) {
    return _api->f_readdir(dp, fno);
}
extern "C" FRESULT f_findfirst(DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern) {
    return _api->f_findfirst(dp, fno, path, pattern);
}
extern "C" FRESULT f_findnext(DIR* dp, FILINFO* fno) {
    return _api->f_findnext(dp, fno);
}
extern "C" int f_puts(const TCHAR* str, FIL* cp) {
    return _api->f_puts(str, cp);
}
