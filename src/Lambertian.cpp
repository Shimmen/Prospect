#include "Lambertian.h"

void
Lambertian::Init(int materialID)
{
	program = ShaderSystem::AddProgram("material/lambertian", materialID);

	if (*program)
	{
		// TODO: Really, this needs to be redone every time the shader is reloaded..! :o
		// MAYBE a better way would be to have fixed locations for all the uniforms?!
		// MAYBE this funtion (i.e. Init()) should be called for every dependent material
		// to that shader program. Sort of how programs can dependent on shaders.
		diffuseTextureLocation = glGetUniformLocation(*program, "u_diffuse");
	}
}

void
Lambertian::BindUniforms() const
{
	// Don't 'use' program, this should be done before
	//glUseProgram(*program);

	const GLuint unit = 0;
	glBindTextureUnit(unit, diffuseTexture);
	glUniform1i(diffuseTextureLocation, unit);
}
