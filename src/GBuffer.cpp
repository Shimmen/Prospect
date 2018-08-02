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
	// Create new textures first, so that we always get new handles, which is an indication
	// to some systems that the texture has changes.
	GLuint oldAlbedo = albedoTexture;
	GLuint oldNormal = normalTexture;
	GLuint oldDepth = depthTexture;

	albedoTexture = CreateFlatTexture(width, height, GL_RGBA8);
	normalTexture = CreateFlatTexture(width, height, GL_RGBA8);
	depthTexture = CreateFlatTexture(width, height, GL_DEPTH_COMPONENT32F);

	// Setup the swizzle for the depth textures so all color channels are depth
	GLenum depthSwizzle[] = { GL_RED, GL_RED, GL_RED, GL_ALPHA };
	glTextureParameteriv(depthTexture, GL_TEXTURE_SWIZZLE_RGBA, (GLint *)depthSwizzle);
	
	// Docs: "glDeleteTextures silently ignores 0's and names that do not correspond to existing textures."
	glDeleteTextures(1, &oldAlbedo);
	glDeleteTextures(1, &oldNormal);
	glDeleteTextures(1, &oldDepth);
}

void
GBuffer::BindForReading()
{
	// TODO!
}
