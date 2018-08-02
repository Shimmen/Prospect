#pragma once

#include <glad/glad.h>

struct GBuffer
{
	// RGBA8: RGB - albedo, A - unused
	GLuint albedoTexture = 0;

	// RGBA8: RGB - normal xyz, A - unused
	GLuint normalTexture = 0;

	// DEPTH_COMPONENT32: projected non-linear depth
	GLuint depthTexture = 0;

	////////////////////////////

	// Create or recreate the g-buffer textures with the given dimensions
	void RecreateGpuResources(int width, int height);

	// Binds the g-buffer textures to uniforms for reading in light shaders
	void BindForReading();

};
