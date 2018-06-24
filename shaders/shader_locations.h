#ifndef UNIFORM_LOCATIONS_H
#define UNIFORM_LOCATIONS_H

// Camera related
#define BINDING_CameraUniformBlock 0

// G-Buffer
#define LOC_u_g_buffer_albedo 20
#define LOC_u_g_buffer_normal 21

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

//

#define PredefinedUniformBlockBinding(name) BINDING_##name
#define PredefinedUniformBlock(name) layout(std140, binding = PredefinedUniformBlockBinding(name)) uniform name

#endif // UNIFORM_LOCATIONS_H