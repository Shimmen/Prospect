#ifndef COMPILATION_H
#define COMPILATION_H

// GL_core_profile is always defined in GLSL:
// https://www.khronos.org/opengl/wiki/Core_Language_(GLSL)#Preprocessor_directives
#ifdef GL_core_profile
 #define COMPILING_CPP  0
 #define COMPILING_GLSL 1
#else
 #define COMPILING_CPP  1
 #define COMPILING_GLSL 0
#endif

#endif // COMPILATION_H
