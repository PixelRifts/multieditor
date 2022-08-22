#include "base/log.h"
#include "os/os.h"
#include "base/vmath.h"
#include "opt/render_2d.h"
#include "opt/ui.h"
#include <math.h>
#include <stdlib.h>

typedef struct psys_particle {
	vec2 pos;
	vec2 vel;
	vec2 acc;
	vec4 color;
	vec4 color_vel;
	f32 lifetime;
} psys_particle;

typedef struct psys_file {
	psys_particle blueprint;
	psys_particle variance;
	f32 speed;
} psys_file;

static M_Arena arena = {0};
static string fp = {0};
static psys_file data = {0};
static f32 timer = 0.f;

#define ParticlePoolSize 128
static psys_particle particles[ParticlePoolSize] = {0};
static b8 particles_used[ParticlePoolSize] = {0};

dll_export string_array Extensions(M_Arena* arena) {
	string exts[] = {
		str_lit("psys")
	};
	return string_static_array_make(arena, exts, 1);
}

f32 random_float_pm(f32 r) {
	srand(OS_TimeMicrosecondsNow() * 2654435761);
	f32 zerotoone = (f32)rand() / (f32)RAND_MAX;
	f32 minusonetoone = (zerotoone - 0.5f) * 2.f;
	return minusonetoone * r;
}

dll_export void Init(string filepath) {
	MemoryZero(particles_used, ParticlePoolSize * sizeof(b8));
	UI_SetColorProperty(ColorProperty_Slider_Base, (vec4) { 0.4f, 0.4f, 0.4f, 1.f });
	UI_SetColorProperty(ColorProperty_Slider_BobBase, (vec4) { 0.5f, 0.5f, 0.5f, 1.f });
	UI_SetColorProperty(ColorProperty_Slider_BobHover, (vec4) { 0.6f, 0.6f, 0.6f, 1.f });
	UI_SetColorProperty(ColorProperty_Slider_BobDrag, (vec4) { 0.3f, 0.3f, 0.3f, 1.f });
	arena_init(&arena);
	fp = filepath;
	string strdata = OS_FileRead(&arena, filepath);
	if (strdata.size == sizeof(psys_file)) {
		memmove(&data, strdata.str, sizeof(psys_file));
	} else {
		data.blueprint = (psys_particle) {
			.pos = (vec2) { 0.f, 0.f },
			.vel = (vec2) { 0.f, -50.f },
			.acc = (vec2) { 0.f, 50.f },
			.color = Color_Red,
			.color_vel = (vec4) { 0.f, 0.f, 0.f, 0.f },
			.lifetime = 4.f
		};
		data.variance = (psys_particle) {
			.pos = (vec2) { 0.f, 0.f },
			.vel = (vec2) { 10.f, 2.f },
			.acc = (vec2) { 0.f, 2.f },
			.color = (vec4) { 0.1f, 0.0f, 0.02f, 0.f },
			.color_vel = (vec4) { 0.f, 0.f, 0.f, 0.f },
			.lifetime = 0.1f
		};
		data.speed = 0.1f;
	}
}

dll_export void Update(f32 dt) {
	timer += dt;
	if (timer >= data.speed) {
		for (u32 i = 0; i < ParticlePoolSize; i++) {
			if (!particles_used[i]) {
				particles_used[i] = true;
				particles[i] = (psys_particle) {
					.pos = (vec2) {
						data.blueprint.pos.x + random_float_pm(data.variance.pos.x),
						data.blueprint.pos.y + random_float_pm(data.variance.pos.y),
					},
					.vel = (vec2) {
						data.blueprint.vel.x + random_float_pm(data.variance.vel.x),
						data.blueprint.vel.y + random_float_pm(data.variance.vel.y),
					},
					.acc = (vec2) {
						data.blueprint.acc.x + random_float_pm(data.variance.acc.x),
						data.blueprint.acc.y + random_float_pm(data.variance.acc.y),
					},
					.color = (vec4) {
						data.blueprint.color.x + random_float_pm(data.variance.color.x),
						data.blueprint.color.y + random_float_pm(data.variance.color.y),
						data.blueprint.color.z + random_float_pm(data.variance.color.z),
						data.blueprint.color.w + random_float_pm(data.variance.color.w),
					},
					.color_vel = (vec4) {
						data.blueprint.color_vel.x + random_float_pm(data.variance.color_vel.x),
						data.blueprint.color_vel.y + random_float_pm(data.variance.color_vel.y),
						data.blueprint.color_vel.z + random_float_pm(data.variance.color_vel.z),
						data.blueprint.color_vel.w + random_float_pm(data.variance.color_vel.w),
					},
					.lifetime = data.blueprint.lifetime + random_float_pm(data.variance.lifetime),
				};
				break;
			}
		}
		timer = 0.f;
	}
	
	for (u32 i = 0; i < ParticlePoolSize; i++) {
		if (particles_used[i]) {
			particles[i].pos.x += particles[i].vel.x * dt;
			particles[i].pos.y += particles[i].vel.y * dt;
			particles[i].vel.x += particles[i].acc.x * dt;
			particles[i].vel.y += particles[i].acc.y * dt;
			particles[i].color.x += particles[i].color_vel.x * dt;
			particles[i].color.y += particles[i].color_vel.y * dt;
			particles[i].color.z += particles[i].color_vel.z * dt;
			particles[i].color.w += particles[i].color_vel.w * dt;
			
			particles[i].lifetime -= dt;
			if (particles[i].lifetime <= 0.f) {
				particles_used[i] = false;
			}
		}
	}
}

dll_export void Render(R2D_Renderer* renderer) {
	UI_Label((vec2) { 10, 40 }, str_lit("Blueprint: "));
	UI_Slider((rect) { 200, 40, 100, 5 }, (vec2) { 10, 20 }, -200.f, 200.f, &data.blueprint.pos.x);
	UI_Label((vec2) { 200, 30 }, str_lit("x"));
	UI_Slider((rect) { 310, 40, 100, 5 }, (vec2) { 10, 20 }, -200.f, 200.f, &data.blueprint.pos.y);
	UI_Label((vec2) { 310, 30 }, str_lit("y"));
	UI_Slider((rect) { 420, 40, 100, 5 }, (vec2) { 10, 20 }, 0.f, 1.f, &data.blueprint.color.x);
	UI_Label((vec2) { 420, 30 }, str_lit("r"));
	UI_Slider((rect) { 530, 40, 100, 5 }, (vec2) { 10, 20 }, 0.f, 1.f, &data.blueprint.color.y);
	UI_Label((vec2) { 530, 30 }, str_lit("g"));
	UI_Slider((rect) { 640, 40, 100, 5 }, (vec2) { 10, 20 }, 0.f, 1.f, &data.blueprint.color.z);
	UI_Label((vec2) { 640, 30 }, str_lit("b"));
	UI_Slider((rect) { 750, 40, 100, 5 }, (vec2) { 10, 20 }, 0.f, 1.f, &data.blueprint.color.w);
	UI_Label((vec2) { 750, 30 }, str_lit("a"));
	
	UI_Label((vec2) { 10, 80 }, str_lit("Variance: "));
	UI_Slider((rect) { 200, 80, 100, 5 }, (vec2) { 10, 20 }, -200.f, 200.f, &data.variance.pos.x);
	UI_Label((vec2) { 200, 70 }, str_lit("x"));
	UI_Slider((rect) { 310, 80, 100, 5 }, (vec2) { 10, 20 }, -200.f, 200.f, &data.variance.pos.y);
	UI_Label((vec2) { 310, 70 }, str_lit("y"));
	UI_Slider((rect) { 420, 80, 100, 5 }, (vec2) { 10, 20 }, 0.f, 1.f, &data.variance.color.x);
	UI_Label((vec2) { 420, 70 }, str_lit("r"));
	UI_Slider((rect) { 530, 80, 100, 5 }, (vec2) { 10, 20 }, 0.f, 1.f, &data.variance.color.y);
	UI_Label((vec2) { 530, 70 }, str_lit("g"));
	UI_Slider((rect) { 640, 80, 100, 5 }, (vec2) { 10, 20 }, 0.f, 1.f, &data.variance.color.z);
	UI_Label((vec2) { 640, 70 }, str_lit("b"));
	UI_Slider((rect) { 750, 80, 100, 5 }, (vec2) { 10, 20 }, 0.f, 1.f, &data.variance.color.w);
	UI_Label((vec2) { 750, 70 }, str_lit("a"));
	
	vec2 old_offset = R2D_PushOffset(renderer, (vec2) { 400.f, 400.f });
	for (u32 i = 0; i < ParticlePoolSize; i++) {
		if (particles_used[i]) {
			R2D_DrawQuadC(renderer, (rect) { particles[i].pos.x, particles[i].pos.y, 10, 10 },
						  particles[i].color, 1);
		}
	}
	R2D_PopOffset(renderer, old_offset);
}

dll_export void Free() {
	string packed_data = {
		.str = (u8*) &data,
		.size = sizeof(psys_file),
	};
	OS_FileWrite(fp, packed_data);
	arena_free(&arena);
}
