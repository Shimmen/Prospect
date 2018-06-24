#include "GBuffer.h"

//
// Internal API
//

GLuint CreateFlatTexture(int width, int height, GLenum internalFormat)
{
	GLuint texture;
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);
	glTextureStorage2D(texture, 1, internalFormat, width, height);

	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_REPEAT);

	return texture;
}

//
// Public API
//

void
GBuffer::RecreateGpuResources(int width, int height)
{
	// Docs: "glDeleteTextures silently ignores 0's and names that do not correspond to existing textures."
	glDeleteTextures(1, &albedoTexture);
	glDeleteTextures(1, &normalTexture);
	glDeleteTextures(1, &depthTexture);

	albedoTexture = CreateFlatTexture(width, height, GL_RGBA8);
	normalTexture = CreateFlatTexture(width, height, GL_RGBA8);
	depthTexture = CreateFlatTexture(width, height, GL_DEPTH_COMPONENT32F);
}

void
GBuffer::BindForReading()
{
	// TODO!
}
