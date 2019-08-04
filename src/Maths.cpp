#include "Maths.h"

#include <cmath>

glm::vec3
SrgbColor(float r, float g, float b)
{
	glm::vec3 sRGB = glm::pow({ r, g, b }, glm::vec3(2.2f));
	return sRGB;
}

void
ExtractFrustumPlanes(const glm::mat4 & matrix, std::array<glm::vec4, 6>& planes)
{
	// From the paper "Fast Extraction of Viewing Frustum Planes from the WorldView-Projection Matrix"
	// https://www.gamedevs.org/uploads/fast-extraction-viewing-frustum-planes-from-world-view-projection-matrixpdf

	// Left clipping plane
	planes[0].x = matrix[0][3] + matrix[0][0];
	planes[0].y = matrix[1][3] + matrix[1][0];
	planes[0].z = matrix[2][3] + matrix[2][0];
	planes[0].w = matrix[3][3] + matrix[3][0];

	// Right clipping plane
	planes[1].x = matrix[0][3] - matrix[0][0];
	planes[1].y = matrix[1][3] - matrix[1][0];
	planes[1].z = matrix[2][3] - matrix[2][0];
	planes[1].w = matrix[3][3] - matrix[3][0];

	// Top clipping plane
	planes[2].x = matrix[0][3] - matrix[0][1];
	planes[2].y = matrix[1][3] - matrix[1][1];
	planes[2].z = matrix[2][3] - matrix[2][1];
	planes[2].w = matrix[3][3] - matrix[3][1];

	// Bottom clipping plane
	planes[3].x = matrix[0][3] + matrix[0][1];
	planes[3].y = matrix[1][3] + matrix[1][1];
	planes[3].z = matrix[2][3] + matrix[2][1];
	planes[3].w = matrix[3][3] + matrix[3][1];

	// Near clipping plane
	planes[4].x = matrix[0][3] + matrix[0][2];
	planes[4].y = matrix[1][3] + matrix[1][2];
	planes[4].z = matrix[2][3] + matrix[2][2];
	planes[4].w = matrix[3][3] + matrix[3][2];

	// Far clipping plane
	planes[5].x = matrix[0][3] - matrix[0][2];
	planes[5].y = matrix[1][3] - matrix[1][2];
	planes[5].z = matrix[2][3] - matrix[2][2];
	planes[5].w = matrix[3][3] - matrix[3][2];

	// Normalize planes (so that we can measure actual distances)
	NormalizePlane(planes[0]);
	NormalizePlane(planes[1]);
	NormalizePlane(planes[2]);
	NormalizePlane(planes[3]);
	NormalizePlane(planes[4]);
	NormalizePlane(planes[5]);
}

void
NormalizePlane(glm::vec4& plane)
{
	float length = std::sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z);
	plane *= 1.0f / length;
}

float
VectorMaxComponent(const glm::vec3& vector)
{
	return std::max(std::max(vector.x, vector.y), vector.z);
}

bool
InPositiveHalfSpace(const glm::vec4& plane, const BoundingSphere& boundingSphere, float epsilon)
{
	float signedDistance = glm::dot(plane, glm::vec4(boundingSphere.center, 1.0f));
	return signedDistance + boundingSphere.radius + epsilon >= 0.0f;
}

bool
InsideFrustum(std::array<glm::vec4, 6>& planes, const BoundingSphere& boundingSphere)
{
	if (!InPositiveHalfSpace(planes[0], boundingSphere)) return false;
	if (!InPositiveHalfSpace(planes[1], boundingSphere)) return false;
	if (!InPositiveHalfSpace(planes[2], boundingSphere)) return false;
	if (!InPositiveHalfSpace(planes[3], boundingSphere)) return false;
	if (!InPositiveHalfSpace(planes[4], boundingSphere)) return false;
	if (!InPositiveHalfSpace(planes[5], boundingSphere)) return false;
	return true;
}

glm::vec2
Halton(int index, int baseX, int baseY)
{
	// Code is a modified version of the Halton sequence generation from
	// https://blog.demofox.org/2017/05/29/when-random-numbers-are-too-random-low-discrepancy-sequences/
	
	glm::vec2 sample{};

	// x axis
	{
		float denominator = float(baseX);
		size_t n = index;
		while (n > 0)
		{
			size_t multiplier = n % baseX;
			sample.x += float(multiplier) / denominator;
			n = n / baseX;
			denominator *= baseX;
		}
	}

	// y axis
	{
		float denominator = float(baseY);
		size_t n = index;
		while (n > 0)
		{
			size_t multiplier = n % baseY;
			sample.y += float(multiplier) / denominator;
			n = n / baseY;
			denominator *= baseY;
		}
	}

	return sample;
}
