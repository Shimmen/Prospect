#pragma once

 #include <cstdio>
 #include <stdlib.h>

 #define Log(...) fprintf(stdout, __VA_ARGS__)
 #define LogError(...) do { fprintf(stderr, __VA_ARGS__); /*getchar();*/ exit(EXIT_FAILURE); } while (false)
