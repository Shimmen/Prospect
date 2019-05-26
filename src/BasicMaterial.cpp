#include "BasicMaterial.h"

#include <glm/gtc/type_ptr.hpp>

BasicMaterial::BasicMaterial()
{
	ShaderSystem::AddProgram("material/basic", this);
}

void
BasicMaterial::ProgramLoaded(GLuint program)
{
	this->program = program;
	if (program)
	{
		modelMatrixLocation = glGetUniformLocation(program, "u_world_from_local");
		prevModelMatrixLocation = glGetUniformLocation(program, "u_prev_world_from_local");
		normalMatrixLocation = glGetUniformLocation(program, "u_world_from_tangent");

		baseColorLocation = glGetUniformLocation(program, "u_base_color");
		roughnessLocation = glGetUniformLocation(program, "u_roughness");
		metallicLocation = glGetUniformLocation(program, "u_metallic");
	}
}

void
BasicMaterial::BindUniforms(Transform& transform, const Transform& prevTransform) const
{
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(transform.matrix));
	glUniformMatrix4fv(prevModelMatrixLocation, 1, GL_FALSE, glm::value_ptr(prevTransform.matrix));
	glUniformMatrix3fv(normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(transform.normalMatrix));

	glUniform3fv(baseColorLocation, 1, glm::value_ptr(baseColor));
	glUniform1f(roughnessLocation, roughness);
	glUniform1f(metallicLocation, metallic);
}
