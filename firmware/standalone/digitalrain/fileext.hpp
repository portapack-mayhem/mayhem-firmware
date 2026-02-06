
extern "C" FRESULT f_open(FIL* fp, const TCHAR* path, BYTE mode);
extern "C" FRESULT f_close(FIL* fp);
extern "C" FRESULT f_read(FIL* fp, void* buff, UINT btr, UINT* br);
extern "C" FRESULT f_write(FIL* fp, const void* buff, UINT btw, UINT* bw);
extern "C" FRESULT f_lseek(FIL* fp, FSIZE_t ofs);
extern "C" FRESULT f_truncate(FIL* fp);

extern "C" FRESULT f_sync(FIL* fp);
extern "C" FRESULT f_opendir(DIR* dp, const TCHAR* path);
extern "C" FRESULT f_closedir(DIR* dp);
extern "C" FRESULT f_readdir(DIR* dp, FILINFO* fno);

extern "C" FRESULT f_findfirst(DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern);
extern "C" FRESULT f_findnext(DIR* dp, FILINFO* fno);

extern "C" FRESULT f_mkdir(const TCHAR* path);
extern "C" FRESULT f_unlink(const TCHAR* path);
extern "C" FRESULT f_rename(const TCHAR* path_old, const TCHAR* path_new);
extern "C" FRESULT f_stat(const TCHAR* path, FILINFO* fno);

extern "C" FRESULT f_chmod(const TCHAR* path, BYTE attr, BYTE mask);
extern "C" FRESULT f_utime(const TCHAR* path, const FILINFO* fno);
extern "C" FRESULT f_chdir(const TCHAR* path);
extern "C" FRESULT f_chdrive(const TCHAR* path);

extern "C" FRESULT f_getcwd(TCHAR* buff, UINT len);
extern "C" FRESULT f_getfree(const TCHAR* path, DWORD* nclst, FATFS** fatfs);
extern "C" FRESULT f_getlabel(const TCHAR* path, TCHAR* label, DWORD* vsn);
extern "C" FRESULT f_setlabel(const TCHAR* label);
extern "C" FRESULT f_forward(FIL* fp, UINT (*func)(const BYTE*, UINT), UINT btf, UINT* bf);
extern "C" FRESULT f_expand(FIL* fp, FSIZE_t szf, BYTE opt);

extern "C" FRESULT f_mount(FATFS* fs, const TCHAR* path, BYTE opt);
extern "C" FRESULT f_mkfs(const TCHAR* path, BYTE opt, DWORD au, void* work, UINT len);

extern "C" FRESULT f_fdisk(BYTE pdrv, const DWORD* szt, void* work);
extern "C" int f_putc(TCHAR c, FIL* fp);

extern "C" int f_puts(const TCHAR* str, FIL* cp);
extern "C" int f_printf(FIL* fp, const TCHAR* str, ...);
extern "C" TCHAR* f_gets(TCHAR* buff, int len, FIL* fp);
