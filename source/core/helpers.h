/* date = August 21st 2022 9:57 am */

#ifndef HELPERS_H
#define HELPERS_H

#include "defines.h"
#include "resources.h"

dll_plugin_api R_Buffer H_LoadObjToBufferVN(string file, u32* count);
dll_plugin_api R_Buffer H_LoadObjToBufferVNCs(string file, u32* count, vec4 color);

#endif //HELPERS_H
