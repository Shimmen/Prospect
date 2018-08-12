#pragma once

#include <glad/glad.h>

struct LightBuffer
{
	int width;
	int height;

	GLuint framebuffer;

	// RGB32F: RGB - accumulated light contribution
	GLuint lightTexture = 0;

	////////////////////////////

	// Create or recreate the light buffer textures with the given dimensions
	void RecreateGpuResources(int width, int height);

};
