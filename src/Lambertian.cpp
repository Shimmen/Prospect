#include "Lambertian.h"

void
Lambertian::Init()
{
	shader = ShaderSystem::AddProgram("material/lambertian");
	diffuseTextureLocation = glGetUniformLocation(*shader, "u_diffuse");
}

void
Lambertian::BindUniforms() const
{
	glUseProgram(*shader);

	const GLuint unit = 0;
	glBindTextureUnit(unit, diffuseTexture);
	glUniform1i(diffuseTextureLocation, unit);
}
