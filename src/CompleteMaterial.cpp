#include "CompleteMaterial.h"

#include <glm/gtc/type_ptr.hpp>

#include "TextureSystem.h"

CompleteMaterial::CompleteMaterial()
{
	ShaderSystem::AddProgram("material/complete", this);
}

void
CompleteMaterial::ProgramLoaded(GLuint program)
{
	this->program = program;
	if (program)
	{
		modelMatrixLocation = glGetUniformLocation(program, "u_world_from_local");
		normalMatrixLocation = glGetUniformLocation(program, "u_world_from_tangent");

		baseColorLocation = glGetUniformLocation(program, "u_base_color");
		roughnessLocation = glGetUniformLocation(program, "u_roughness");
		metallicLocation = glGetUniformLocation(program, "u_metallic");
		normalMapLocation = glGetUniformLocation(program, "u_normal_map");
	}
}

void
CompleteMaterial::BindUniforms(Transform& transform) const
{
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(transform.matrix));
	glUniformMatrix3fv(normalMatrixLocation, 1, GL_FALSE, glm::value_ptr(transform.normalMatrix));

	if (!baseColorTexture)
	{
		baseColorTexture = TextureSystem::LoadLdrImage("assets/default/base_color.png");
	}

	const GLuint baseColorUnit = 0;
	glBindTextureUnit(baseColorUnit, baseColorTexture);
	glUniform1i(baseColorLocation, baseColorUnit);

	if (!normalMap)
	{
		normalMap = TextureSystem::LoadDataTexture("assets/default/normal.png");
	}

	const GLuint normalMapUnit = 1;
	glBindTextureUnit(normalMapUnit, normalMap);
	glUniform1i(normalMapLocation, normalMapUnit);

	glUniform1f(roughnessLocation, roughness);
	glUniform1f(metallicLocation, metallic);
}
