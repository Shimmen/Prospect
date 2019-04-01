#include "GBuffer.h"

#include "Logging.h"
#include "TextureSystem.h"

#include "shader_locations.h"

void
GBuffer::RecreateGpuResources(int width, int height)
{
	this->width = width;
	this->height = height;

	// Docs: "glDeleteTextures silently ignores 0's and names that do not correspond to existing textures."
	glDeleteTextures(1, &albedoTexture);
	glDeleteTextures(1, &materialTexture);
	glDeleteTextures(1, &normalTexture);
	glDeleteTextures(1, &depthTexture);

	albedoTexture = TextureSystem::CreateTexture(width, height, GL_RGBA8, GL_NEAREST, GL_NEAREST);
	materialTexture = TextureSystem::CreateTexture(width, height, GL_RGBA8, GL_NEAREST, GL_NEAREST);
	normalTexture = TextureSystem::CreateTexture(width, height, GL_RGBA16F, GL_NEAREST, GL_NEAREST);
	depthTexture = TextureSystem::CreateTexture(width, height, GL_DEPTH_COMPONENT32F, GL_NEAREST, GL_NEAREST);

	// Setup the swizzle for the depth textures so all color channels are depth
	GLenum depthSwizzle[] = { GL_RED, GL_RED, GL_RED, GL_ALPHA };
	glTextureParameteriv(depthTexture, GL_TEXTURE_SWIZZLE_RGBA, (GLint *)depthSwizzle);

	if (!framebuffer)
	{
		glCreateFramebuffers(1, &framebuffer);

		GLenum drawBuffers[] = {
			PredefinedOutputLocation(o_g_buffer_albedo),
			PredefinedOutputLocation(o_g_buffer_material),
			PredefinedOutputLocation(o_g_buffer_normal)
		};
		int numDrawBuffers = sizeof(drawBuffers) / sizeof(GLenum);
		glNamedFramebufferDrawBuffers(framebuffer, numDrawBuffers, drawBuffers);
	}

	GLenum albedoAttachment = PredefinedOutputLocation(o_g_buffer_albedo);
	glNamedFramebufferTexture(framebuffer, albedoAttachment, albedoTexture, 0);

	int materialAttachment = PredefinedOutputLocation(o_g_buffer_material);
	glNamedFramebufferTexture(framebuffer, materialAttachment, materialTexture, 0);

	int normalAttachment = PredefinedOutputLocation(o_g_buffer_normal);
	glNamedFramebufferTexture(framebuffer, normalAttachment, normalTexture, 0);

	glNamedFramebufferTexture(framebuffer, GL_DEPTH_ATTACHMENT, depthTexture, 0);

	GLenum status = glCheckNamedFramebufferStatus(framebuffer, GL_DRAW_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		LogError("The G-buffer framebuffer is not complete!");
	}
}
