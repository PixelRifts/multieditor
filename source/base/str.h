/* date = September 27th 2021 3:04 pm */

#ifndef STR_H
#define STR_H

#include <string.h>
#include "mem.h"

typedef struct string_const {
    u8* str;
    u64 size;
} string_const;
typedef string_const string;

typedef struct string_const_list_node {
    string str;
    struct string_const_list_node* next;
} string_const_list_node;
typedef string_const_list_node string_list_node;

typedef struct string_const_list {
    string_const_list_node* first;
    string_const_list_node* last;
    i32 node_count;
    u64 total_size;
} string_const_list;
typedef string_const_list string_list;

typedef struct string_const_array {
    u32 cap;
    u32 len;
    string* elems;
} string_const_array;
typedef string_const_array string_array;

dll_plugin_api string_const_array string_static_array_make(M_Arena* arena, string* strings, u32 count);
dll_plugin_api void string_array_add(string_const_array* array, string data);
dll_plugin_api string_const string_array_remove(string_const_array* array, int idx);
dll_plugin_api void string_array_free(string_const_array* array);

//-

#define str_lit(s) (string_const) { .str = (u8*)(s), .size = sizeof(s) - 1 }
#define str_expand(s) (i32)(s).size, (s).str

dll_plugin_api string_const str_alloc(M_Arena* arena, u64 size); // NOTE(EVERYONE): this will try to get one extra byte for \0
dll_plugin_api string_const str_copy(M_Arena* arena, string_const other);
dll_plugin_api string_const str_cat(M_Arena* arena, string_const a, string_const b);
dll_plugin_api string_const str_from_format(M_Arena* arena, const char* format, ...);
dll_plugin_api string_const str_replace_all(M_Arena* arena, string_const to_fix, string_const needle, string_const replacement);
dll_plugin_api u64 str_substr_count(string_const str, string_const needle);
dll_plugin_api u64 str_find_first(string_const str, string_const needle, u32 offset);
dll_plugin_api u64 str_find_last(string_const str, string_const needle, u32 offset);
dll_plugin_api u32 str_hash(string_const str);

dll_plugin_api b8 str_eq(string_const a, string_const b);

dll_plugin_api void string_list_push_node(string_const_list* list, string_const_list_node* node);
dll_plugin_api void string_list_push(M_Arena* arena, string_const_list* list, string_const str);
dll_plugin_api b8   string_list_equals(string_const_list* a, string_const_list* b);
dll_plugin_api b8   string_list_contains(string_const_list* a, string_const needle);
dll_plugin_api string_const string_list_flatten(M_Arena* arena, string_const_list* list);

//- Encoding Stuff 

typedef struct string_utf16_const {
    u16* str;
    u64 size;
} string_utf16_const;
typedef string_utf16_const string_utf16;

dll_plugin_api string_utf16_const str16_cstring(u16 *cstr);
dll_plugin_api string_utf16_const str16_from_str8(M_Arena *arena, string_const str);
dll_plugin_api string_const str8_from_str16(M_Arena *arena, string_utf16_const str);

#endif //STR_H
