#ifndef SHADER_LOCATIONS_H
#define SHADER_LOCATIONS_H

///////////////////////////////////////////////////////////////////////////////
// Attributes

#define PredefinedAttributeLocation(name) ATTRIB_##name
#define PredefinedAttribute(type, name) layout(location = PredefinedAttributeLocation(name)) in type name

//

#define ATTRIB_a_position  0
#define ATTRIB_a_normal    1
#define ATTRIB_a_tex_coord 2
#define ATTRIB_a_tangent   3

///////////////////////////////////////////////////////////////////////////////
// Outputs / render targets

// (i.e. if we're not in a shader/GLSL environment)
#ifdef GL_COLOR_ATTACHMENT0
 #define PredefinedOutputLocation(name) GL_COLOR_ATTACHMENT0 + TARGET_##name
#endif

#define PredefinedOutputShaderLocation(name) TARGET_##name
#define PredefinedOutput(type, name) layout(location = PredefinedOutputShaderLocation(name)) out type name

//

// (for all "normal" color outputs)
#define TARGET_o_color 0

#define TARGET_o_g_buffer_albedo   0
#define TARGET_o_g_buffer_material 1
#define TARGET_o_g_buffer_norm_vel 2

///////////////////////////////////////////////////////////////////////////////
// Image locations

#define PredefinedImageBinding(name) IMG_LOC_##name

#define PredefinedNoiseImage(var) layout(binding = IMG_LOC_BlueNoiseImage, r8) \
                                    restrict readonly uniform image2DArray var

//

#define IMG_LOC_BlueNoiseImage 7

///////////////////////////////////////////////////////////////////////////////
// Uniform locations

#define PredefinedUniformLocation(name) LOC_##name
#define PredefinedUniform(type, name) layout(location = PredefinedUniformLocation(name)) uniform type name

//

#define LOC_u_g_buffer_albedo   20
#define LOC_u_g_buffer_material 21
#define LOC_u_g_buffer_norm_vel 22
#define LOC_u_g_buffer_depth    23

#define LOC_u_gui_projection 40
#define LOC_u_gui_texture    41

#define LOC_u_texture    0
#define LOC_u_shadow_map 1

#define LOC_u_world_from_local      101
#define LOC_u_projection_from_world 102

///////////////////////////////////////////////////////////////////////////////
// Uniform block bindings

#define PredefinedUniformBlockType(name) TYPE_##name
#define PredefinedUniformBlockBinding(name) BINDING_##name

#define PredefinedUniformBlockRaw(name) layout(std140, binding = PredefinedUniformBlockBinding(name)) uniform name
#define PredefinedUniformBlock(name, var) layout(std140, binding = PredefinedUniformBlockBinding(name)) uniform name \
                                                    { PredefinedUniformBlockType(name) var; }

//

#define TYPE_CameraUniformBlock CameraUniforms
#define BINDING_CameraUniformBlock 0

#define TYPE_SceneUniformBlock SceneUniforms
#define BINDING_SceneUniformBlock 1

#define BINDING_ShadowMapSegmentBlock 2

#define TYPE_DirectionalLightBlock DirectionalLight
#define BINDING_DirectionalLightBlock 3

#define TYPE_SSAODataBlock SSAOData
#define BINDING_SSAODataBlock 4

#define BINDING_SphereSampleBuffer 10

///////////////////////////////////////////////////////////////////////////////

#endif // SHADER_LOCATIONS_H
