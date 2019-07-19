#version 460

#include <common.glsl>
#include <etc/pointcloud_data.h>

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;

layout(std140, binding = 13) uniform PointcloudBlock
{
    PointcloudData data;
};

out float v_distance_from_eye;
out vec3 v_color;

// From https://twitter.com/Donzanoid/status/616370134278606848
// (Draw triangle strips with 14 vertices to make it work)
vec3 createCube(in uint vertexId)
{
    uint b = 1 << vertexId;
    return vec3((0x287a & b) != 0, (0x02af & b) != 0, (0x31e3 & b) != 0) - vec3(0.5);
}

void main()
{
    vec3 posFromEye = position - data.camera_position.xyz;
    float sqDist = dot(posFromEye, posFromEye);

    vec3 offset = position;
    //offset.y += 0.2 * cos(3.0 * data.time + sqDist / 30.0);

    float scale = data.base_scale;// + square(sin(8.0 * data.time)) * 0.005;

    vec3 pos = scale * createCube(gl_VertexID) + offset;
    gl_Position = data.projection_from_world * vec4(pos, 1.0);

    v_distance_from_eye = sqrt(sqDist);
    if (dot(color.rgb, color.rgb) < 0.01) {
        // (there is missing color for some points in some data sets..)
        v_color = vec3(1, 0, 1);
    } else {
        v_color = color.rgb * clamp(color.a, 0.25, 1.0);
    }
}
