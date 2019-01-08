#include "BloomPass.h"

#include <imgui.h>

#include "GuiSystem.h"
#include "ShaderSystem.h"

#include "shader_locations.h"

void
BloomPass::Draw(const LightBuffer& lightBuffer)
{
	static bool isSetup = false;
	if (!isSetup)
	{
		Setup();
		isSetup = true;
	}

	GLuint highPassed = GetHighPassedLightBuffer(lightBuffer, threshold);

	if (ImGui::CollapsingHeader("Bloom"))
	{
		ImGui::SliderFloat("Threshold", &threshold, 0.0f, 100.0f);
		GuiSystem::Texture(highPassed);
	}

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	for (int i = 0; i < 10; ++i)
	{
		glUseProgram(*blurHorizontalProgram);
		{
			if (i == 0)
			{
				glBindTextureUnit(0, highPassed);
			}
			else
			{
				glBindTextureUnit(0, bloomTextures[1]);
			}

			int size = blurBaseSize;
			for (int lod = 0; lod < numBlurLevels; ++lod)
			{
				int fbIdx = lod;
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers[fbIdx]);
				glViewport(0, 0, size, size);

				glUniform1f(blurHtextureSizeLoc, float(size));
				glUniform1f(blurHtextureLodLoc, float(lod));

				glBindVertexArray(emptyVertexArray);
				glDrawArrays(GL_TRIANGLES, 0, 3);

				size /= 2;
			}
		}

		glUseProgram(*blurVerticalProgram);
		{
			glBindTextureUnit(0, bloomTextures[0]);

			int size = blurBaseSize;
			for (int lod = 0; lod < numBlurLevels; ++lod)
			{
				int fbIdx = numBlurLevels + lod;
				glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffers[fbIdx]);
				glViewport(0, 0, size, size);

				glUniform1f(blurVtextureSizeLoc, float(size));
				glUniform1f(blurVtextureLodLoc, float(lod));

				glBindVertexArray(emptyVertexArray);
				glDrawArrays(GL_TRIANGLES, 0, 3);

				size /= 2;
			}
		}
	}

	glEnable(GL_DEPTH_TEST);
}

void BloomPass::ProgramLoaded(GLuint program)
{
	if (blurVerticalProgram && program == *blurVerticalProgram)
	{
		glProgramUniform1i(program, PredefinedUniformLocation(u_texture), 0);
		blurVtextureSizeLoc = glGetUniformLocation(program, "u_texture_size");
		blurVtextureLodLoc = glGetUniformLocation(program, "u_texture_lod");
	}

	if (blurHorizontalProgram && program == *blurHorizontalProgram)
	{
		glProgramUniform1i(program, PredefinedUniformLocation(u_texture), 0);
		blurHtextureSizeLoc = glGetUniformLocation(program, "u_texture_size");
		blurHtextureLodLoc = glGetUniformLocation(program, "u_texture_lod");
	}

	if (highPassProgram && program == *highPassProgram)
	{
		glProgramUniform1i(program, PredefinedUniformLocation(u_texture), 0);
		highPassTresholdLoc = glGetUniformLocation(program, "u_threshold");
	}
}

GLuint BloomPass::GetHighPassedLightBuffer(const LightBuffer& lightBuffer, float threshold)
{	
	glUseProgram(*highPassProgram);
	
	glBindImageTexture(0, highPassedTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGBA16F);
	glBindTextureUnit(0, lightBuffer.lightTexture);

	//static float lastThreshold = -1.0f;
	//if (threshold != lastThreshold)
	{
		glProgramUniform1f(*highPassProgram, highPassTresholdLoc, threshold);
		//lastThreshold = threshold;
	}
	
	const int localSize = 32;
	glDispatchCompute(blurBaseSize / localSize, blurBaseSize / localSize, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	glGenerateTextureMipmap(highPassedTexture);

	return highPassedTexture;
}

void BloomPass::Setup()
{
	glCreateVertexArrays(1, &emptyVertexArray);

	ShaderSystem::AddComputeProgram(&highPassProgram, "post/high_pass.comp.glsl", this);
	ShaderSystem::AddProgram(&blurVerticalProgram, "quad.vert.glsl", "post/blur_v.frag.glsl", this);
	ShaderSystem::AddProgram(&blurHorizontalProgram, "quad.vert.glsl", "post/blur_h.frag.glsl", this);

	// Setup texture for storing high passed intermediate data
	{
		glCreateTextures(GL_TEXTURE_2D, 1, &highPassedTexture);

		glTextureParameteri(highPassedTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(highPassedTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(highPassedTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(highPassedTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTextureStorage2D(highPassedTexture, numBlurLevels, GL_RGBA16F, blurBaseSize, blurBaseSize);
	}

	// Setup texture and framebuffer for performing the bloom
	
	int numFramebuffers = 2 * numBlurLevels;
	framebuffers.resize(numFramebuffers);
	glCreateFramebuffers(numFramebuffers, framebuffers.data());

	glCreateTextures(GL_TEXTURE_2D, 2, bloomTextures);
	for (int i = 0; i < 2; ++i)
	{
		glTextureStorage2D(bloomTextures[i], numBlurLevels, GL_RGBA16F, blurBaseSize, blurBaseSize);

		glTextureParameteri(bloomTextures[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
		glTextureParameteri(bloomTextures[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(bloomTextures[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(bloomTextures[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
		for (int k = 0; k < numBlurLevels; ++k)
		{
			int fbIdx = numBlurLevels * i + k;
			glNamedFramebufferTexture(framebuffers[fbIdx], GL_COLOR_ATTACHMENT0, bloomTextures[i], k);
		}
	}

	// Make alias for the bloom results texture
	bloomResults = bloomTextures[1];
}