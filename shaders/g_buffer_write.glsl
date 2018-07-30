#ifndef G_BUFFER_WRITE_H
#define G_BUFFER_WRITE_H

#include <shader_locations.h>

// TODO: Maybe remove this? Unnecessary files and indirection?
// Same with g_buffer_read maybe, although we want more utilites with that...
PredefinedOutput(vec4, o_g_buffer_albedo);
PredefinedOutput(vec4, o_g_buffer_normal);

#endif // G_BUFFER_WRITE_H
