/* date = March 11th 2022 2:07 pm */

#ifndef UTILS_H
#define UTILS_H

#include "defines.h"
#include "mem.h"
#include "str.h"

//~ Time

typedef u64 U_DenseTime;

typedef struct U_DateTime {
    u16 ms;
    u8  sec;
    u8  minute;
    u8  hour;
    u8  day;
    u8  month;
    s32 year;
} U_DateTime;

dll_plugin_api U_DenseTime U_DenseTimeFromDateTime(U_DateTime* datetime);
dll_plugin_api U_DateTime  U_DateTimeFromDenseTime(U_DenseTime densetime);

//~ Filepaths

dll_plugin_api string U_FixFilepath(M_Arena* arena, string filepath);
dll_plugin_api string U_GetFullFilepath(M_Arena* arena, string filename);
dll_plugin_api string U_GetFilenameFromFilepath(string filepath);
dll_plugin_api string U_GetDirectoryFromFilepath(string filepath);
dll_plugin_api string U_RemoveExtensionFromFilename(string filename);
dll_plugin_api string U_GetExtensionFromFilepath(string filepath);

#endif //UTILS_H
