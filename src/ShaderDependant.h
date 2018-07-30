#pragma once

#include <glad/glad.h>

class ShaderDepandant
{
public:

	//
	// Called every time the program is loaded and reloaded
	//
	virtual void ProgramLoaded(GLuint program) = 0;

};
