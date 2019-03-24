#include "FinalPass.h"

#include <imgui.h>

#include "GuiSystem.h"
#include "ShaderSystem.h"
#include "UniformValue.h"
#include "FullscreenQuad.h"

#include "shader_locations.h"
#include <imgui_internal.h>

void
FinalPass::Draw(const LightBuffer& lightBuffer, BloomPass& bloomPass)
{

/*
	if (!logLumProgram)
	{
		ShaderSystem::AddComputeProgram(&logLumProgram, "post/log_luminance.comp.glsl", this);
	}

	{
		glBindImageTexture(0, lightBuffer.lightTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_RGB16F);
		glBindImageTexture(1, logLumTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);

		glUseProgram(*logLumProgram);
		glDispatchCompute(4, 4, 1);

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glGenerateTextureMipmap(logLumTexture);

		// TODO!!
	}
*/

	if (!finalProgram)
	{
		ShaderSystem::AddProgram(&finalProgram, "quad.vert.glsl", "post/final.frag.glsl", this);
	}

	{
		static Uniform<float> exposure("u_exposure", 5.0f);
		static Uniform<float> vignette("u_vignette_falloff", 0.25f);
		static Uniform<float> gamma("u_gamma", 2.2f);
		static Uniform<float> bloomAmount("u_bloom_amount", 0.04f);

		if (ImGui::CollapsingHeader("Postprocess"))
		{
			ImGui::SliderFloat("Exposure", &exposure.value, 0.0f, 32.0f, "%.1f");
			ImGui::SliderFloat("Vignette amount", &vignette.value, 0.0f, 2.0f, "%.2f");
			ImGui::SliderFloat("Gamma", &gamma.value, 0.1f, 4.0f, "%.1f");
			ImGui::VerticalSeparator();
			ImGui::SliderFloat("Bloom blur radius", &bloomPass.blurRadius, 0.0f, 0.02f, "%.6f", 3.0f);
			ImGui::SliderFloat("Bloom amount", &bloomAmount.value, 0.0f, 1.0f, "%.3f");
		}

		exposure.UpdateUniformIfNeeded(*finalProgram);
		vignette.UpdateUniformIfNeeded(*finalProgram);
		gamma.UpdateUniformIfNeeded(*finalProgram);
		bloomAmount.UpdateUniformIfNeeded(*finalProgram);
	}

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(*finalProgram);
	{
		glBindTextureUnit(0, lightBuffer.lightTexture);
		glBindTextureUnit(1, bloomPass.bloomResults);

		FullscreenQuad::Draw();
	}

	glEnable(GL_DEPTH_TEST);
}

void FinalPass::ProgramLoaded(GLuint program)
{
	if (finalProgram && program == *finalProgram)
	{
		glProgramUniform1i(*finalProgram, PredefinedUniformLocation(u_texture), 0);
		glProgramUniform1i(*finalProgram, glGetUniformLocation(*finalProgram, "u_bloom_texture"), 1);
	}
}


