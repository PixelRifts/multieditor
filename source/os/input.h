/* date = July 15th 2022 5:07 pm */

#ifndef INPUT_H
#define INPUT_H

#define Input_Press 1
#define Input_Release 2
#define Input_Repeat 3

#ifdef PLATFORM_WIN
#  include "impl/win32_key_codes.h"
#else
#  error "Not Implemented YET"
#endif

void __OS_InputKeyCallback(u8 key, i32 action);
void __OS_InputButtonCallback(u8 button, i32 action);
void __OS_InputCursorPosCallback(f32 xpos, f32 ypos);
void __OS_InputScrollCallback(f32 xscroll, f32 yscroll);
void __OS_InputReset();

dll_plugin_api b32 OS_InputKey(i32 key);
dll_plugin_api b32 OS_InputKeyPressed(i32 key);
dll_plugin_api b32 OS_InputKeyReleased(i32 key);
dll_plugin_api b32 OS_InputKeyHeld(i32 key);
dll_plugin_api b32 OS_InputButton(i32 button);
dll_plugin_api b32 OS_InputButtonPressed(i32 button);
dll_plugin_api b32 OS_InputButtonReleased(i32 button);
dll_plugin_api f32 OS_InputGetMouseX();
dll_plugin_api f32 OS_InputGetMouseY();
dll_plugin_api f32 OS_InputGetMouseScrollX();
dll_plugin_api f32 OS_InputGetMouseScrollY();
dll_plugin_api f32 OS_InputGetMouseAbsoluteScrollX();
dll_plugin_api f32 OS_InputGetMouseAbsoluteScrollY();
dll_plugin_api f32 OS_InputGetMouseDX();
dll_plugin_api f32 OS_InputGetMouseDY();
dll_plugin_api f32 OS_InputGetMouseRecordedX();
dll_plugin_api f32 OS_InputGetMouseRecordedY();

#endif //INPUT_H
