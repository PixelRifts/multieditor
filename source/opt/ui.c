#include "ui.h"
#include <stdio.h>

Stack_Impl(UI_RegionStack, UI_Region);
b8  void_ptr_null_eq(void* key) { return key == nullptr; }
b8  void_ptr_eq(void* a, void* b) { return a == b; }
u64 void_ptr_hash(void* key) { return (u64)key; }
u64 region_state_null_eq(UI_RegionState s) { return s.v[0] == 0; }
u64 region_state_tombstone_eq(UI_RegionState s) { return s.v[0] == 69; }
HashTable_Impl(UI_RegionStateTable, void_ptr_null_eq, void_ptr_eq, void_ptr_hash,
			   (UI_RegionState){69}, region_state_null_eq, region_state_tombstone_eq);

//~ Properties and State

typedef struct UI_State {
    R2D_Renderer* drawer;
    UI_RegionStack stack;
	
	UI_RegionStateTable_hash_table state_table;
	
	vec4 colors[ColorProperty_Max];
    f32  floats[FloatProperty_Max];
    
    R2D_FontInfo* font;
} UI_State;
static UI_State _ui_state;
static R2D_FontInfo _ui_default_font;


void UI_SetColorProperty(UI_ColorProperty which, vec4 color) {
    _ui_state.colors[which] = color;
}

void UI_SetFloatProperty(UI_FloatProperty which, f32 value) {
    _ui_state.floats[which] = value;
}

//~ Regions

void UI_BeginRegion(UI_Region region) {
    if (_ui_state.stack.len != 0) {
        UI_Region parent = UI_RegionStack_peek(&_ui_state.stack);
        region.x += parent.x;
        region.y += parent.y;
    }
    UI_RegionStack_push(&_ui_state.stack, region);
    R2D_PopCullRect(_ui_state.drawer, region);
    R2D_PopOffset(_ui_state.drawer, (vec2) { region.x, region.y });
}


void UI_EndRegion() {
    UI_RegionStack_pop(&_ui_state.stack);
    UI_Region region = UI_RegionStack_peek(&_ui_state.stack);
    
    R2D_PopCullRect(_ui_state.drawer, region);
    R2D_PopOffset(_ui_state.drawer, (vec2) { region.x, region.y });
}

//~ UI Elements

b8 UI_Button(UI_Region region) {
    UI_Region parent = UI_RegionStack_peek(&_ui_state.stack);
    rect actual = region;
    actual.x += parent.x;
    actual.y += parent.y;
    // Check status
    vec4 color = _ui_state.colors[ColorProperty_Button_Base];
    vec2 mouse_pos = { OS_InputGetMouseX(), OS_InputGetMouseY() };
    b8 click = false;
    if (rect_contains_point(actual, mouse_pos)) {
        if (OS_InputButtonPressed(Input_MouseButton_Left)) {
			click = true;
		} else if (OS_InputButton(Input_MouseButton_Left)) {
			color = _ui_state.colors[ColorProperty_Button_Click];
		} else
			color = _ui_state.colors[ColorProperty_Button_Hover];
	}
	
	R2D_DrawQuadC(_ui_state.drawer, region, color, _ui_state.floats[FloatProperty_Rounding]);
	return click;
}

b8 UI_Checkbox(UI_Region region, b8* value) {
    b8 switched = false;
    vec4 color = _ui_state.colors[ColorProperty_Checkbox_Base];
    vec2 mouse_pos = { OS_InputGetMouseX(), OS_InputGetMouseY() };
    if (rect_contains_point(region, mouse_pos)) {
        if (OS_InputButtonPressed(Input_MouseButton_Left)) {
            *value = !(*value);
            switched = true;
        } else
            color = _ui_state.colors[ColorProperty_Checkbox_Hover];
    }
    R2D_DrawQuadC(_ui_state.drawer, region, color, _ui_state.floats[FloatProperty_Rounding]);
    
    f32 checkbox_padding = _ui_state.floats[FloatProperty_Checkbox_Padding];
    UI_Region inner = (UI_Region) { region.x + checkbox_padding, region.y + checkbox_padding, region.w - 2 * checkbox_padding, region.h - 2 * checkbox_padding };
    
    if (*value)
        R2D_DrawQuadC(_ui_state.drawer, inner, _ui_state.colors[ColorProperty_Checkbox_Inner], _ui_state.floats[FloatProperty_Rounding]);
    return switched;
}

b8 UI_ButtonLabeled(UI_Region region, string label) {
    b8 r = UI_Button(region);
    
    f32 label_size = R2D_GetStringSize(_ui_state.font, label);
    f32 label_loc_x = region.x + (region.w / 2.f) - (label_size / 2.f);
    f32 label_loc_y = region.y + (region.h / 2.f) + (_ui_state.font->font_size / 4.f);
    R2D_DrawString(_ui_state.drawer, _ui_state.font, (vec2) { label_loc_x, label_loc_y }, label);
    
    return r;
}

b8 UI_Slider(UI_Region slider, vec2 bob_size, f32 min, f32 max, f32* value) {
    b8* slider_dragging;
	if (!UI_RegionStateTable_hash_table_get_ptr(&_ui_state.state_table, (void*)value, (UI_RegionState**)&slider_dragging)) {
		UI_RegionStateTable_hash_table_set(&_ui_state.state_table, (void*)value, (UI_RegionState){0});
		UI_RegionStateTable_hash_table_get_ptr(&_ui_state.state_table, (void*)value, (UI_RegionState**)&slider_dragging);
	}
	UI_Region parent = UI_RegionStack_peek(&_ui_state.stack);
    
    // mapping for example 10 to 20 -> 50 to 65
    // 10 -> s, 20 -> t 
    // mapping ratio is: 15 / 10
    // bob_x is: 50 + ((50 - v) * (15/10))
    
    f32 mapping_ratio = (slider.w) / (max - min);
    f32 bob_x = slider.x + ((*value - min) * mapping_ratio) - (bob_size.x / 2.f);
    f32 bob_y = slider.y - (bob_size.y / 2.f) + (slider.h / 2.f);
    
    UI_Region bob = { bob_x, bob_y, bob_size.x, bob_size.y };
    
    rect actual_bob = bob;
    actual_bob.x += parent.x;
    actual_bob.y += parent.y;
    
    vec4 color = _ui_state.colors[ColorProperty_Slider_BobBase];
    
    vec2 mouse_pos = { OS_InputGetMouseX(), OS_InputGetMouseY() };
    if (rect_contains_point(actual_bob, mouse_pos)) {
        color = _ui_state.colors[ColorProperty_Slider_BobHover];
    }
    
    if (OS_InputButtonPressed(Input_MouseButton_Left)) {
        if (rect_contains_point(actual_bob, mouse_pos)) {
            *slider_dragging = true;
        }
    } else if (OS_InputButtonReleased(Input_MouseButton_Left)) {
        *slider_dragging = false;
    }
    
    f32 recorded_mouse_pos_x = OS_InputGetMouseRecordedX();
    if (*slider_dragging) {
        // If started dragging from slider bob
        color = _ui_state.colors[ColorProperty_Slider_BobDrag];
        f32 dx = OS_InputGetMouseDX();
        f32 absx = recorded_mouse_pos_x + dx - (bob_size.x / 2.f);
        absx = Clamp(slider.x - (bob_size.x / 2.f), absx, slider.x + slider.w - (bob_size.x / 2.f));
        actual_bob.x = absx;
        
        // Set actual float value by doing reverse mapping
        f32 reverse_mapping_ratio = (max - min) / (slider.w);
        *value = min + ((actual_bob.x + (bob_size.x / 2.f) - slider.x) * reverse_mapping_ratio);
    }
    
    // Draw
    R2D_DrawQuadC(_ui_state.drawer, slider, _ui_state.colors[ColorProperty_Slider_Base], _ui_state.floats[FloatProperty_Rounding]);
    R2D_DrawQuadC(_ui_state.drawer, actual_bob, color, _ui_state.floats[FloatProperty_Rounding]);
	return *slider_dragging;
}

void UI_Label(vec2 pos, string label) {
    R2D_DrawString(_ui_state.drawer, _ui_state.font, pos, label);
}

//~ Main Procedures

static void UI_DefaultStyle(void) {
    _ui_state.colors[ColorProperty_Button_Base] = (vec4) { 0.8f, 0.2f, 0.3f, 1.0f };
    _ui_state.colors[ColorProperty_Button_Hover] = (vec4) { 0.85f, 0.3f, 0.4f, 1.0f };
    _ui_state.colors[ColorProperty_Button_Click] = (vec4) { 0.55f, 0.1f, 0.2f, 1.0f };
    
    _ui_state.colors[ColorProperty_Checkbox_Base] = (vec4) { 0.8f, 0.2f, 0.3f, 1.0f };
    _ui_state.colors[ColorProperty_Checkbox_Hover] = (vec4) { 0.85f, 0.3f, 0.4f, 1.0f };
    _ui_state.colors[ColorProperty_Checkbox_Inner] = (vec4) { 0.75f, 0.2f, 0.3f, 1.0f };
    
    _ui_state.colors[ColorProperty_Slider_Base] = (vec4) { 0.8f, 0.2f, 0.3f, 1.0f };
    _ui_state.colors[ColorProperty_Slider_BobBase] = (vec4) { 0.70f, 0.15f, 0.2f, 1.0f };
    _ui_state.colors[ColorProperty_Slider_BobHover] = (vec4) { 0.85f, 0.3f, 0.4f, 1.0f };
    _ui_state.colors[ColorProperty_Slider_BobDrag] = (vec4) { 0.75f, 0.2f, 0.3f, 1.0f };
    
    _ui_state.floats[FloatProperty_Rounding] = 3.f;
    _ui_state.floats[FloatProperty_Checkbox_Padding] = 3.f;
    
    _ui_state.font = &_ui_default_font;
}

void UI_Init(R2D_Renderer* drawer) {
    _ui_state.drawer = drawer;
	UI_RegionStateTable_hash_table_init(&_ui_state.state_table);
	// OOF.. new fonts for different font sizes...
	R2D_FontLoad(&_ui_default_font, str_lit("res/Inconsolata.ttf"), 18);
    UI_DefaultStyle();
}

void UI_Free() {
    R2D_FontFree(&_ui_default_font);
    UI_RegionStack_free(&_ui_state.stack);
	UI_RegionStateTable_hash_table_free(&_ui_state.state_table);
}
