/* date = May 5th 2022 3:16 pm */

#ifndef UI_H
#define UI_H

#include "base/vmath.h"
#include "base/mem.h"
#include "base/str.h"
#include "opt/render_2d.h"
#include "os/input.h"
#include "base/ds.h"

typedef rect UI_Region;

typedef struct UI_RegionState {
	u64 v[1];
} UI_RegionState;

Stack_Prototype(UI_RegionStack, UI_Region);
HashTable_Prototype(UI_RegionStateTable, void*, UI_RegionState);

typedef u32 UI_ColorProperty;
enum {
    ColorProperty_Button_Base,
    ColorProperty_Button_Hover,
    ColorProperty_Button_Click,
    
    ColorProperty_Checkbox_Base,
    ColorProperty_Checkbox_Hover,
    ColorProperty_Checkbox_Inner,
    
    ColorProperty_Slider_Base,
    ColorProperty_Slider_BobBase,
    ColorProperty_Slider_BobHover,
    ColorProperty_Slider_BobDrag,
    
    ColorProperty_Max
};

typedef u32 UI_FloatProperty;
enum {
    FloatProperty_Rounding,
    
    FloatProperty_Checkbox_Padding,
    
    FloatProperty_Max
};

dll_plugin_api void UI_SetColorProperty(UI_ColorProperty which, vec4 color);
dll_plugin_api void UI_SetFloatProperty(UI_FloatProperty which, f32 value);

dll_plugin_api void UI_BeginRegion(UI_Region region);
dll_plugin_api void UI_EndRegion();

dll_plugin_api b8 UI_Button(UI_Region region);
dll_plugin_api b8 UI_ButtonLabeled(UI_Region region, string label);
dll_plugin_api b8 UI_Checkbox(UI_Region region, b8* value);
dll_plugin_api b8 UI_Slider(UI_Region slider, vec2 bob_size, f32 min, f32 max, f32* value);
dll_plugin_api void UI_Label(vec2 pos, string label);

dll_plugin_api void UI_Init(R2D_Renderer* drawer);
dll_plugin_api void UI_Free();

#endif //UI_H
