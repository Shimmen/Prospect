#include "Lambertian.h"

#include <glm/gtc/type_ptr.hpp>

void
Lambertian::Init(int thisMaterialID)
{
	program = ShaderSystem::AddProgram("material/lambertian", thisMaterialID);

	if (*program)
	{
		modelMatrixLocation = glGetUniformLocation(*program, "u_world_from_local");
		diffuseTextureLocation = glGetUniformLocation(*program, "u_diffuse");
	}
}

void
Lambertian::BindUniforms(Transform& transform) const
{
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(transform.matrix));

	const GLuint unit = 0;
	glBindTextureUnit(unit, diffuseTexture);
	glUniform1i(diffuseTextureLocation, unit);
}
