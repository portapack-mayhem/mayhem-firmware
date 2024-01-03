#ifndef __UNTAR
#define __UNTAR

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "string_format.hpp"
#include "file.hpp"

class UnTar {
   public:
    static std::filesystem::path untar(std::u16string tar, std::function<void(const std::string)> cb = NULL) {
        File tf;
        auto result = tf.open(tar, true, false);
        if (!result.value().ok()) return "";
        return untar_int(&tf, cb);
    }

   private:
    static int parseoct(const char* p, size_t n) {
        int i = 0;
        while (*p < '0' || *p > '7') {
            ++p;
            --n;
        }
        while (*p >= '0' && *p <= '7' && n > 0) {
            i *= 8;
            i += *p - '0';
            ++p;
            --n;
        }
        return i;
    }

    static bool is_end_of_archive(const char* p) {
        for (int n = 511; n >= 0; --n)
            if (p[n] != '\0') return false;
        return true;
    }

    static size_t strnlen(const char* s, size_t maxlen) {
        for (size_t i = 0; i < maxlen; i++) {
            if (s[i] == '\0')
                return i;
        }
        return maxlen;
    }

    static bool isValidName(char* name) {
        size_t pathStrLen = strnlen(name, 100);
        if (pathStrLen == 0 || pathStrLen >= 100) return false;
        return true;
    }

    static bool create_dir(char* pathname) {
        char* p;
        std::filesystem::filesystem_error r;

        if (!isValidName(pathname)) return false;
        /* Strip trailing '/' */
        if (pathname[strlen(pathname) - 1] == '/')
            pathname[strlen(pathname) - 1] = '\0';

        /* Try creating the directory. */
        std::string dirnameStr = u'/' + pathname;
        std::filesystem::path dirname = dirnameStr;

        r = make_new_directory(dirname);

        if (!r.ok()) {
            /* On failure, try creating parent directory. */
            p = strrchr(pathname, '/');
            if (p != NULL) {
                *p = '\0';
                create_dir(pathname);
                *p = '/';
                r = make_new_directory(dirname);
            }
        }
        return (r.ok());
    }

    static bool verify_checksum(const char* p) {
        int n, u = 0;
        for (n = 0; n < 512; ++n) {
            if (n < 148 || n > 155)
                /* Standard tar checksum adds unsigned bytes. */
                u += ((unsigned char*)p)[n];
            else
                u += 0x20;
        }
        return (u == parseoct(p + 148, 8));
    }

    static std::filesystem::path untar_int(File* a, std::function<void(const std::string)> cb = NULL) {
        char buff[512];
        UINT bytes_read;
        std::string binfile = "";
        std::string fn = "";
        int filesize;
        for (;;) {
            auto readres = a->read(buff, 512);
            if (!readres.is_ok()) return "";
            bytes_read = readres.value();
            if (bytes_read < 512) {
                return "";
            }
            if (is_end_of_archive(buff)) {
                return binfile;
            }
            if (!verify_checksum(buff)) {
                return "";
            }
            filesize = parseoct(buff + 124, 12);
            switch (buff[156]) {
                case '1':
                    break;
                case '2':
                    break;
                case '3':
                    break;
                case '4':
                    break;
                case '5':
                    create_dir(buff);
                    filesize = 0;
                    break;
                case '6':
                    break;
                default:
                    if (isValidName(buff)) {
                        fn = buff;
                        if (fn.length() > 5 && fn.substr(fn.length() - 4) == ".bin") {
                            binfile = fn;
                        }
                        if (cb != NULL) cb(fn);
                    } else {
                        return "";  // bad tar
                    }
                    break;
            }
            File f;
            if (filesize > 0) {
                delete_file(fn);
                auto fres = f.open(fn, false, true);
                if (!fres.value().ok()) return "";
            }
            while (filesize > 0) {
                readres = a->read(buff, 512);
                if (!readres.is_ok()) return "";
                bytes_read = readres.value();
                if (bytes_read < 512) {
                    return "";
                }
                if (filesize < 512)
                    bytes_read = filesize;
                auto fwres = f.write(buff, bytes_read);
                if (!fwres.is_ok()) return "";
                filesize -= bytes_read;
                f.sync();
            }
        }
        return binfile;
    }
};
#endif