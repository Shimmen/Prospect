#include "FinalPass.h"

#include <imgui.h>

#include "GuiSystem.h"
#include "PerformOnce.h"
#include "ShaderSystem.h"
#include "UniformValue.h"
#include "TextureSystem.h"
#include "FullscreenQuad.h"

#include "shader_locations.h"
#include "shader_constants.h"

void
FinalPass::Draw(const GBuffer& gBuffer, const LightBuffer& lightBuffer, Scene& scene)
{
	PerformOnce(logLumTexture = TextureSystem::CreateTexture(1024, 1024, GL_R32F));
	PerformOnce(currentLumTexture = TextureSystem::CreateTexture(1, 1, GL_R32F));

	PerformOnce(ShaderSystem::AddComputeProgram(&logLumProgram, "post/log_luminance.comp.glsl", this));
	{
		glBindTextureUnit(0, lightBuffer.lightTexture);
		glBindImageTexture(1, logLumTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R32F);

		glUseProgram(*logLumProgram);
		glDispatchCompute(32, 32, 1); //(32 * 32 = 1024)

		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		glGenerateTextureMipmap(logLumTexture);
	}

	PerformOnce(ShaderSystem::AddComputeProgram(&exposureProgram, "post/expose.comp.glsl", this));
	{
		int xGroups = int(ceil(lightBuffer.width / 32.0f));
		int yGroups = int(ceil(lightBuffer.height / 32.0f));

		glBindImageTexture(0, lightBuffer.lightTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, logLumTexture, 10, GL_FALSE, 0, GL_READ_ONLY, GL_R32F);
		glBindImageTexture(2, currentLumTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32F);

		glUseProgram(*exposureProgram);
		glDispatchCompute(xGroups, yGroups, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	taaPass.Draw(lightBuffer, gBuffer);
	bloomPass.Draw(lightBuffer);

	PerformOnce(ShaderSystem::AddProgram(&finalProgram, "quad.vert.glsl", "post/final.frag.glsl", this));
	{
		static Uniform<float> vignette("u_vignette_falloff", 0.25f);
		static Uniform<float> gamma("u_gamma", 2.2f);
		static Uniform<float> bloomAmount("u_bloom_amount", 0.04f);
		static Uniform<int>   tonemapOperator("u_tonemap_operator_selector", TONEMAP_OP_ACES);

		if (ImGui::CollapsingHeader("Postprocess"))
		{
			ImGui::SliderFloat("Vignette amount", &vignette.value, 0.0f, 2.0f, "%.2f");
			ImGui::SliderFloat("Bloom blend", &bloomAmount.value, 0.0f, 1.0f, "%.6f", 4.0f);
			ImGui::SliderFloat("Gamma", &gamma.value, 0.1f, 4.0f, "%.1f");

			// Tonemapping operator combo box selector
			{
				// NOTE: This has to line up with the tonemapping operators in shader_constants.glsl. Obviously not
				// very nice or neat, but I don't intend on changing these much or adding more operators. This feature
				// mostly exist so it's possible to compare them a bit etc... It will work for now! :)
				const char *items[] = { "ACES", "Reinhard", "Uncharted 2", "Clamp" };

				if (ImGui::BeginCombo("Tonemapping operator", items[tonemapOperator.value]))
				{
					for (int i = 0; i < IM_ARRAYSIZE(items); ++i)
					{
						bool isSelected = i == tonemapOperator.value;
						if (ImGui::Selectable(items[i], isSelected))
						{
							tonemapOperator.value = i;
						}
						if (isSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
			}

			if (ImGui::TreeNode("Camera"))
			{
				scene.mainCamera->DrawEditorGui();
				ImGui::TreePop();
			}
		}

		vignette.UpdateUniformIfNeeded(*finalProgram);
		gamma.UpdateUniformIfNeeded(*finalProgram);
		bloomAmount.UpdateUniformIfNeeded(*finalProgram);
		tonemapOperator.UpdateUniformIfNeeded(*finalProgram);
	}

	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(*finalProgram);
	{
		glBindTextureUnit(0, taaPass.outputTexture);	
		glBindTextureUnit(1, bloomPass.bloomResults);
		glBindTextureUnit(2, logLumTexture);

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
