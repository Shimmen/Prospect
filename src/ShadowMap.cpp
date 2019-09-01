#include "ShadowMap.h"

#include "Logging.h"

void
ShadowMap::RecreateGpuResources(int size)
{
	this->size = size;

	glDeleteTextures(1, &texture);
	glCreateTextures(GL_TEXTURE_2D, 1, &texture);

	glTextureStorage2D(texture, 1, GL_DEPTH_COMPONENT32F, size, size);
	glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	GLenum depthSwizzle[] = { GL_RED, GL_RED, GL_RED, GL_ALPHA };
	glTextureParameteriv(texture, GL_TEXTURE_SWIZZLE_RGBA, (GLint *)depthSwizzle);

	if (!framebuffer)
	{
		glCreateFramebuffers(1, &framebuffer);
		glNamedFramebufferDrawBuffer(framebuffer, GL_NONE); // (only depth target!)
	}

	glNamedFramebufferTexture(framebuffer, GL_DEPTH_ATTACHMENT, texture, 0);

	GLenum status = glCheckNamedFramebufferStatus(framebuffer, GL_DRAW_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		LogError("The shadow pass framebuffer is not complete!");
	}

}

/*
void
ShadowMap::AssignShadowMapSegments(std::vector<DirectionalLight>& dirLights)
{
	size_t totalLightCount = dirLights.size();

	glm::mat4 toUv = glm::translate(vec3(0.5f)) * glm::scale(vec3(0.5f));

	if (totalLightCount <= 4)
	{
		glm::mat4 uvScale = glm::scale(glm::vec3(0.5f));
		glm::mat4 uvBias = glm::translate(glm::vec3(0.5f));

		int mid = size / 2;

		// TODO Make some automatic way of calculating the UV transform!
		// i.e. Just by specifying the min and max xy and the total size!

		ShadowMapSegment segments[4] = {
			{ 0, mid, 0, mid, uvScale * toUv, glm::mat4() },
			{ 0, mid, mid, size, uvBias * uvScale * toUv, glm::mat4() },
			{ 0, mid, 0, mid, uvBias * uvScale * toUv, glm::mat4() },
			{ 0, mid, 0, mid, uvBias * uvScale * toUv, glm::mat4() }
		};
	}
	else
	{
		// TODO: Handle this case as well!
		assert(false);
	}

}
*/