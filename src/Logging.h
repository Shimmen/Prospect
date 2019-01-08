#pragma once

// _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

#if defined(_MSC_VER) && defined(_DEBUG) && false

 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif // WIN32_LEAN_AND_MEAN
 #include <Windows.h>

 constexpr int _DEBUG_PRINT_BUFFER_SIZE = 1024 * 1024;
 static char _debugPrintBuffer[_DEBUG_PRINT_BUFFER_SIZE] = { 0 };

 #define Log(...) do { sprintf(_debugPrintBuffer, __VA_ARGS__); OutputDebugString(_debugPrintBuffer); } while (false)
 #define LogError(...) do { Log(__VA_ARGS__); DebugBreak(); } while (false)

#else

 #include <cstdio>
 #include <stdlib.h>

 #define Log(...) fprintf(stdout, __VA_ARGS__)
 #define LogError(...) do { fprintf(stderr, __VA_ARGS__); /*getchar();*/ exit(EXIT_FAILURE); } while (false)

#endif
