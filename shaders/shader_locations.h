#ifndef SHADER_LOCATIONS_H
#define SHADER_LOCATIONS_H

///////////////////////////////////////////////////////////////////////////////
// Attributes

#define PredefinedAttributeLocation(name) ATTRIB_##name
#define PredefinedAttribte(type, name) layout(location = PredefinedAttributeLocation(name)) in type name

//

#define ATTRIB_a_position  0
#define ATTRIB_a_normal    1
#define ATTRIB_a_tex_coord 2

///////////////////////////////////////////////////////////////////////////////
// Outputs / render targets

#ifdef GL_COLOR_ATTACHMENT0
 #define PredefinedOutputLocation(name) GL_COLOR_ATTACHMENT0 + TARGET_##name
#endif

#define PredefinedOutputShaderLocation(name) TARGET_##name
#define PredefinedOutput(type, name) layout(location = PredefinedOutputShaderLocation(name)) out type name

//

// (for all "normal" color outputs)
#define TARGET_o_color 0

#define TARGET_o_g_buffer_albedo 0
#define TARGET_o_g_buffer_normal 1

///////////////////////////////////////////////////////////////////////////////
// Uniform locations

#define PredefinedUniformLocation(name) LOC_##name
#define PredefinedUniform(type, name) layout(location = PredefinedUniformLocation(name)) uniform type name

//

#define LOC_u_g_buffer_albedo 20
#define LOC_u_g_buffer_normal 21
#define LOC_u_g_buffer_depth  22

#define LOC_u_directional_light_color     30
#define LOC_u_directional_light_direction 31

#define LOC_u_gui_projection 40
#define LOC_u_gui_texture    41

#define LOC_u_texture 0

#define LOC_u_world_from_local 1
#define LOC_u_projection_from_world 2

///////////////////////////////////////////////////////////////////////////////
// Uniform block bindings

#define PredefinedUniformBlockBinding(name) BINDING_##name
#define PredefinedUniformBlock(name) layout(std140, binding = PredefinedUniformBlockBinding(name)) uniform name

//

#define BINDING_CameraUniformBlock 0

#define BINDING_ShadowMapSegmentBlock 1

#define BINDING_DirectionalLightBlock 2

///////////////////////////////////////////////////////////////////////////////

#endif // SHADER_LOCATIONS_H
