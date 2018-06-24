#ifndef UNIFORM_LOCATIONS_H
#define UNIFORM_LOCATIONS_H

// Camera related
#define LOC_u_world_from_local     10
#define LOC_u_view_from_world      11
#define LOC_u_projection_from_view 12

// G-Buffer
#define LOC_u_g_buffer_albedo       20
#define LOC_u_g_buffer_normal_depth 21

#define TARGET_o_g_buffer_albedo 0
#define TARGET_o_g_buffer_normal 1

// Directional light
#define LOC_u_directional_light_color     30
#define LOC_u_directional_light_direction 31

//

#ifdef GL_COLOR_ATTACHMENT0
 #define PredefinedOutputLocation(name) GL_COLOR_ATTACHMENT0 + TARGET_##name
#endif

#define PredefinedOutputShaderLocation(name) TARGET_##name
#define PredefinedOutput(type, name) layout(location = PredefinedOutputShaderLocation(name)) out type name

//

#define PredefinedUniformLocation(name) LOC_##name
#define PredefinedUniform(type, name) layout(location = PredefinedUniformLocation(name)) uniform type name

#endif // UNIFORM_LOCATIONS_H
