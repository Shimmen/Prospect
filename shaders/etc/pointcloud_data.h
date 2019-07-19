#ifndef POINTCLOUD_DATA_H
#define POINTCLOUD_DATA_H

struct PointcloudData
{
    mat4 projection_from_world;
    vec4 camera_position;

    float time;

    float base_scale;
    float a, b;


};

#endif // POINTCLOUD_DATA_H
