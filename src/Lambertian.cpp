#include "Lambertian.h"

#include <glm/gtc/type_ptr.hpp>

Lambertian::Lambertian()
{
	ShaderSystem::AddProgram("material/lambertian", this);
}

void
Lambertian::ProgramLoaded(GLuint program)
{
	this->program = program;
	if (program)
	{
		modelMatrixLocation = glGetUniformLocation(program, "u_world_from_local");
		normalMatrixLocation = glGetUniformLocation(program, "u_world_from_tangent");
		diffuseTextureLocation = glGetUniformLocation(program, "u_diffuse");
	}
}

void
Lambertian::BindUniforms(Transform& transform) const
{
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(transform.matrix));
	glUniformMatrix3fv(normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(transform.normalMatrix));

	const GLuint unit = 0;
	glBindTextureUnit(unit, diffuseTexture);
	glUniform1i(diffuseTextureLocation, unit);
}
