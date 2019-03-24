#include "BloomPass.h"

#include "GuiSystem.h"
#include "ShaderSystem.h"
#include "FullscreenQuad.h"

#include "shader_locations.h"

void
BloomPass::Draw(const LightBuffer& lightBuffer)
{
	static int lastW = 0;
	static int lastH = 0;
	if (lastW != lightBuffer.width || lastH != lightBuffer.height)
	{
		Setup(lightBuffer.width, lightBuffer.height);
		lastW = lightBuffer.width;
		lastH = lightBuffer.height;
	}

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	// Render light buffer to mip0 of the sampling texture
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, downsamplingFramebuffers[0]);
		glUseProgram(*blitProgram);
		glBindTextureUnit(0, lightBuffer.lightTexture);
		FullscreenQuad::Draw();
	}

	glBindTextureUnit(0, downsamplingTexture);

	static std::vector<glm::ivec2> targetSizes;
	targetSizes.resize(numDownsamples + 1);

	targetSizes[0] = { lightBuffer.width, lightBuffer.height };

	// Iteratively downsample down to the lowest mip level
	{
		glUseProgram(*downsampleProgram);

		for (int targetMip = 1; targetMip <= numDownsamples; ++targetMip)
		{
			// See answer for NPOT texture mip sizes: https://computergraphics.stackexchange.com/a/1444
			// I.e., use integer division and discard the rest. Using NPOT textures won't be optimal for
			// this task, but hopefully it works fine anyway.
			targetSizes[targetMip] = targetSizes[targetMip - 1] / glm::ivec2(2);
			
			int width = targetSizes[targetMip].x;
			int height = targetSizes[targetMip].y;

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, downsamplingFramebuffers[targetMip]);
			glViewport(0, 0, width, height);

			glProgramUniform2f(*downsampleProgram, dsTargetTexelSizeLoc, 1.0f / width, 1.0f / height);
			glProgramUniform1i(*downsampleProgram, dsTargetLodLoc, targetMip);

			FullscreenQuad::Draw();
		}
	}

	// Iteratively upsample back to mip0
	{
		glUseProgram(*upsampleProgram);
		glProgramUniform1f(*upsampleProgram, usBlurRadiusLoc, blurRadius);

		for (int targetMip = numDownsamples - 1; targetMip >= 0; --targetMip)
		{
			if (targetMip == numDownsamples - 1)
			{
				glProgramUniform1i(*upsampleProgram, glGetUniformLocation(*upsampleProgram, "u_texture_to_blur"), 0);
			}
			else
			{
				// Yes, this is the same texture that we draw to but we don't draw to the same mip as we read from
				glProgramUniform1i(*upsampleProgram, glGetUniformLocation(*upsampleProgram, "u_texture_to_blur"), 1);
				glBindTextureUnit(1, upsamplingTexture);
			}

			int width = targetSizes[targetMip].x;
			int height = targetSizes[targetMip].y;

			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, upsamplingFramebuffers[targetMip]);
			glViewport(0, 0, width, height);

			glProgramUniform1f(*upsampleProgram, usTexelAspectLoc, float(width) / float(height));
			glProgramUniform1i(*upsampleProgram, usTargetLodLoc, targetMip);

			FullscreenQuad::Draw();
		}
	}

	glEnable(GL_DEPTH_TEST);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	// Make alias for the bloom results texture
	bloomResults = upsamplingTexture;
}

void BloomPass::Setup(int width, int height)
{
	ShaderSystem::AddProgram(&blitProgram, "quad.vert.glsl", "blit.frag.glsl", this);
	ShaderSystem::AddProgram(&downsampleProgram, "quad.vert.glsl", "post/bloom_downsample.frag.glsl", this);
	ShaderSystem::AddProgram(&upsampleProgram, "quad.vert.glsl", "post/bloom_upsample.frag.glsl", this);

	int numLevelsNeeded = numDownsamples + 1;

	glCreateTextures(GL_TEXTURE_2D, 1, &downsamplingTexture);
	{
		glTextureStorage2D(downsamplingTexture, numLevelsNeeded, GL_RGBA16F, width, height);

		glTextureParameteri(downsamplingTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTextureParameteri(downsamplingTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(downsamplingTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(downsamplingTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	// Need to write to each of the levels for the initial copy and subsequent resamplings
	downsamplingFramebuffers.resize(numLevelsNeeded);
	glCreateFramebuffers(numLevelsNeeded, downsamplingFramebuffers.data());

	for (int level = 0; level < numLevelsNeeded; ++level)
	{
		glNamedFramebufferTexture(downsamplingFramebuffers[level], GL_COLOR_ATTACHMENT0, downsamplingTexture, level);
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &upsamplingTexture);
	{
		glTextureStorage2D(upsamplingTexture, numLevelsNeeded, GL_RGBA16F, width, height);

		glTextureParameteri(upsamplingTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTextureParameteri(upsamplingTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		glTextureParameteri(upsamplingTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(upsamplingTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	// Need to write to each of the levels except for the lowest mip
	int numUpsamplings = numLevelsNeeded - 1;
	upsamplingFramebuffers.resize(numUpsamplings);
	glCreateFramebuffers(numUpsamplings, upsamplingFramebuffers.data());

	for (int level = 0; level < numUpsamplings; ++level)
	{
		glNamedFramebufferTexture(upsamplingFramebuffers[level], GL_COLOR_ATTACHMENT0, upsamplingTexture, level);
	}
}

void BloomPass::ProgramLoaded(GLuint program)
{
	if (blitProgram && program == *blitProgram)
	{
		glProgramUniform1i(program, PredefinedUniformLocation(u_texture), 0);
	}

	if (downsampleProgram && program == *downsampleProgram)
	{
		glProgramUniform1i(program, PredefinedUniformLocation(u_texture), 0);
		dsTargetTexelSizeLoc = glGetUniformLocation(program, "u_target_texel_size");
		dsTargetLodLoc = glGetUniformLocation(program, "u_target_lod");
	}

	if (upsampleProgram && program == *upsampleProgram)
	{
		glProgramUniform1i(program, PredefinedUniformLocation(u_texture), 0);
		usTexelAspectLoc = glGetUniformLocation(program, "u_texel_aspect");
		usBlurRadiusLoc = glGetUniformLocation(program, "u_blur_radius");
		usTargetLodLoc = glGetUniformLocation(program, "u_target_lod");
	}

}
