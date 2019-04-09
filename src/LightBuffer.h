#pragma once

#include <glad/glad.h>

#include <GBuffer.h>

struct LightBuffer
{
	int width;
	int height;

	GLuint framebuffer;

	// RGBA32F: RGB - accumulated light contribution, A - unused, for now
	GLuint lightTexture = 0;

	////////////////////////////

	// Create or recreate the light buffer textures with the given dimensions
	void RecreateGpuResources(int width, int height, const GBuffer& gBuffer);

};
