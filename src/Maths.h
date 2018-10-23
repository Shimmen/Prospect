#pragma once

#include <glm/glm.hpp>

#include <array>

struct BoundingSphere
{
	glm::vec3 center;
	float radius;
};

void ExtractFrustumPlanes(const glm::mat4& matrix, std::array<glm::vec4, 6>& planes);
void NormalizePlane(glm::vec4& plane);

float VectorMaxComponent(const glm::vec3& vector);

bool InPositiveHalfSpace(const glm::vec4& plane, const BoundingSphere& boundingSphere, float epsilon = 0.01f);
bool InsideFrustum(std::array<glm::vec4, 6>& planes, const BoundingSphere& boundingSphere);
