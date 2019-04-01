#include "FinalPass.h"

#include <imgui.h>

#include "GuiSystem.h"
#include "ShaderSystem.h"
#include "UniformValue.h"
#include "TextureSystem.h"
#include "FullscreenQuad.h"

#include "shader_locations.h"

void
FinalPass::Draw(const LightBuffer& lightBuffer, BloomPass& bloomPass, Scene& scene)
{
	if (!logLumProgram)
	{
		ShaderSystem::AddComputeProgram(&logLumProgram, "post/log_luminance.comp.glsl", this);
	}

	if (!logLumTexture)
	{
		logLumTexture = TextureSystem::CreateTexture(1024, 1024, GL_R16F);
	}

	{
		glBindTextureUnit(0, lightBuffer.lightTexture);
		glBindImageTexture(1, logLumTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);

		glUseProgram(*logLumProgram);
		glDispatchCompute(32, 32, 1); //(32 * 32 = 1024)

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glGenerateTextureMipmap(logLumTexture);
	}

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
			ImGui::Spacing();
			ImGui::SliderFloat("Bloom blur radius", &bloomPass.blurRadius, 0.0f, 0.02f, "%.6f", 3.0f);
			ImGui::SliderFloat("Bloom amount", &bloomAmount.value, 0.0f, 1.0f, "%.3f");

			if (ImGui::TreeNode("Camera"))
			{
				scene.mainCamera->DrawEditorGui();
				ImGui::TreePop();
			}
		}

		exposure.UpdateUniformIfNeeded(*finalProgram);
		vignette.UpdateUniformIfNeeded(*finalProgram);
		gamma.UpdateUniformIfNeeded(*finalProgram);
		bloomAmount.UpdateUniformIfNeeded(*finalProgram);
	}

	if (!currentLumTexture)
	{
		currentLumTexture = TextureSystem::CreateTexture(1, 1, GL_R32F, GL_NEAREST, GL_NEAREST);
	}

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(*finalProgram);
	{
		glBindTextureUnit(0, lightBuffer.lightTexture);
		glBindTextureUnit(1, bloomPass.bloomResults);
		glBindTextureUnit(2, logLumTexture);

		glBindImageTexture(0, currentLumTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

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
		glProgramUniform1i(*finalProgram, glGetUniformLocation(*finalProgram, "u_avg_log_lum_texture"), 2);
	}
}


