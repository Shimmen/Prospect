#ifndef CAMERA_UNIFORMS_H
#define CAMERA_UNIFORMS_H

struct CameraUniforms
{
    mat4 view_from_world;
	mat4 world_from_view;
    mat4 projection_from_view;
    mat4 view_from_projection;
    vec4 near_far; // x=near, y=far, z=(far / (far - near)), w=((-far * near) / (far - near))

    float iso;
    float aperture;
    float shutter_speed;
	float delta_time;
};

#endif // CAMERA_UNIFORMS_H
