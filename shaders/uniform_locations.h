#ifndef UNIFORM_LOCATIONS_H
#define UNIFORM_LOCATIONS_H

#define LOC_u_diffuse            0

#define LOC_u_world_from_local     10
#define LOC_u_view_from_world      11
#define LOC_u_projection_from_view 12

#define PredefinedUniformLocation(name) LOC_##name
#define PredefinedUniform(type, name) layout(location = PredefinedUniformLocation(name)) uniform type name

#endif // UNIFORM_LOCATIONS_H
