#ifndef CAMERA_UNIFORMS_H
#define CAMERA_UNIFORMS_H

#include <compilation.h>

#if COMPILING_CPP
 #define mat4 glm::mat4
 #define vec4 glm::vec4
#endif

struct CameraUniforms
{
    mat4 view_from_world;
	mat4 world_from_view;
    mat4 projection_from_view;
    mat4 view_from_projection;
    vec4 near_far; // x=near, y=far, z=(far / (far - near)), w=((-far * near) / (far - near))
};

#if COMPILING_CPP
 #undef mat4
 #undef vec4
#endif

#endif // CAMERA_UNIFORMS_H
