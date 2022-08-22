/* date = March 31st 2022 1:23 pm */

#ifndef OS_H
#define OS_H

#include "base/mem.h"
#include "base/str.h"
#include "base/utils.h"

//~ OS Init

dll_plugin_api void OS_Init(void);

//~ TLS

dll_plugin_api void  OS_ThreadContextSet(void* ctx);
dll_plugin_api void* OS_ThreadContextGet(void);

//~ Memory

dll_plugin_api void* OS_MemoryReserve(u64 size);
dll_plugin_api void  OS_MemoryCommit(void* memory, u64 size);
dll_plugin_api void  OS_MemoryDecommit(void* memory, u64 size);
dll_plugin_api void  OS_MemoryRelease(void* memory, u64 size);

//~ Files

dll_plugin_api b32    OS_FileCreate(string filename);

dll_plugin_api b32    OS_FileExists(string filename);
dll_plugin_api b32    OS_FileRename(string filename, string new_name);
dll_plugin_api string OS_FileRead(M_Arena* arena, string filename);
dll_plugin_api b32    OS_FileCreateWrite(string filename, string data);
dll_plugin_api b32    OS_FileCreateWrite_List(string filename, string_list data);
dll_plugin_api b32    OS_FileWrite(string filename, string data);
dll_plugin_api b32    OS_FileWrite_List(string filename, string_list data);
dll_plugin_api void   OS_FileOpen(string filename);

dll_plugin_api b32    OS_FileDelete(string filename);

dll_plugin_api b32    OS_FileCreateDir(string dirname);
dll_plugin_api b32    OS_FileDeleteDir(string dirname);
dll_plugin_api void   OS_FileOpenDir(string dirname);

//~ Utility Paths

typedef u32 OS_SystemPath;
enum {
	SystemPath_CurrentDir,
	SystemPath_Binary,
	SystemPath_UserData,
	SystemPath_TempData,
};

dll_plugin_api string OS_Filepath(M_Arena* arena, OS_SystemPath path);

//~ File Properties

typedef u32 OS_DataAccessFlags;
enum {
	DataAccess_Read  = 0x1,
	DataAccess_Write = 0x2,
	DataAccess_Exec  = 0x4,
};

typedef u32 OS_FilePropertyFlags;
enum {
	FileProperty_IsFolder = 0x1,
};

typedef struct OS_FileProperties {
	u64 size;
	U_DenseTime create_time;
	U_DenseTime modify_time;
	OS_FilePropertyFlags flags;
	OS_DataAccessFlags access;
} OS_FileProperties;

dll_plugin_api OS_FileProperties OS_FileGetProperties(string filename);

//~ File Iterator

// Just a big buffer. will be OS specific
typedef struct OS_FileIterator {
	u8 v[640];
} OS_FileIterator;

dll_plugin_api OS_FileIterator OS_FileIterInit(string path);
dll_plugin_api OS_FileIterator OS_FileIterInitPattern(string path);
dll_plugin_api b32  OS_FileIterNext(M_Arena* arena, OS_FileIterator* iter, string* name_out, OS_FileProperties* prop_out);
dll_plugin_api void OS_FileIterEnd(OS_FileIterator* iter);

//~ Time

dll_plugin_api U_DateTime OS_TimeUniversalNow(void);
dll_plugin_api U_DateTime OS_TimeLocalFromUniversal(U_DateTime* date_time);
dll_plugin_api U_DateTime OS_TimeUniversalFromLocal(U_DateTime* date_time);

dll_plugin_api u64  OS_TimeMicrosecondsNow(void);
dll_plugin_api void OS_TimeSleepMilliseconds(u32 t);

//~ Shared Libraries

// Just a buffer. will be OS specific
typedef struct OS_Library {
	u64 v[1];
} OS_Library;

dll_plugin_api OS_Library OS_LibraryLoad(string path);
dll_plugin_api void_func* OS_LibraryGetFunction(OS_Library lib, char* name);
dll_plugin_api void       OS_LibraryRelease(OS_Library lib);

//~ Threading

typedef u32 thread_func(void* context); 

typedef struct OS_Thread {
	u64 v[1];
} OS_Thread;

dll_plugin_api OS_Thread OS_ThreadCreate(thread_func* start, void* context);
dll_plugin_api void      OS_ThreadWaitForJoin(OS_Thread* other);
dll_plugin_api void      OS_ThreadWaitForJoinAll(OS_Thread** threads, u32 count);
dll_plugin_api void      OS_ThreadWaitForJoinAny(OS_Thread** threads, u32 count);

#endif //OS_H
