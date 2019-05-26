#pragma once

#include "Material.h"

#include <glm/glm.hpp>

struct BasicMaterial : public Material
{
	BasicMaterial();
	~BasicMaterial() = default;

	glm::vec3 baseColor;
	float roughness;
	float metallic;

	void ProgramLoaded(GLuint program) override;
	void BindUniforms(Transform& transform, const Transform& prevTransform) const override;

private:

	GLint baseColorLocation;
	GLint roughnessLocation;
	GLint metallicLocation;

	GLint modelMatrixLocation;
	GLint prevModelMatrixLocation;
	GLint normalMatrixLocation;

};
