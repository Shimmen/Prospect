#pragma once

#define PerformOnceVariableName0(line) _macro_perform_once_first_ ## line
#define PerformOnceVariableName(line) PerformOnceVariableName0(line)

#define PerformOnce(expr) \
	static bool PerformOnceVariableName(__LINE__) = true; \
	if (PerformOnceVariableName(__LINE__))                \
	{                                                     \
	    expr;                                             \
	    PerformOnceVariableName(__LINE__) = false;        \
	}
