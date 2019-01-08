#include "FinalPass.h"

#include <imgui.h>

#include "GuiSystem.h"
#include "ShaderSystem.h"

#include "shader_locations.h"

void
FinalPass::Draw(const LightBuffer& lightBuffer, const BloomPass& bloomPass)
{
	if (!emptyVertexArray)
	{
		glCreateVertexArrays(1, &emptyVertexArray);
	}

	if (!finalProgram)
	{
		ShaderSystem::AddProgram("quad.vert.glsl", "post/final.frag.glsl", this);
	}

	{
		float lastExposure = 0.0f;
		static float exposure = 1.0f;

		float lastVignette = 0.0f;
		static float vignette = 0.25f;

		float lastGamma = 0.0f;
		static float gamma = 2.2f;

		static float lastBloomLevels[5] = { 0 };
		static float bloomLevels[5];
		static bool useBloom = false;

		if (ImGui::CollapsingHeader("Postprocess"))
		{
			ImGui::SliderFloat("Exposure", &exposure, 0.0f, 32.0f, "%.1f");
			ImGui::SliderFloat("Vignette amount", &vignette, 0.0f, 2.0f, "%.2f");
			ImGui::SliderFloat("Gamma", &gamma, 0.1f, 4.0f, "%.1f");

			if (ImGui::TreeNode("Bloom levels"))
			{
				ImGui::Checkbox("Use bloom", &useBloom);
				ImGui::Text("Bloom levels:");
				ImGui::SliderFloat("L0", &bloomLevels[0], 0.0f, 1.0f, "%.3f", 3.0f);
				ImGui::SliderFloat("L1", &bloomLevels[1], 0.0f, 1.0f, "%.3f", 3.0f);
				ImGui::SliderFloat("L2", &bloomLevels[2], 0.0f, 1.0f, "%.3f", 3.0f);
				ImGui::SliderFloat("L3", &bloomLevels[3], 0.0f, 1.0f, "%.3f", 3.0f);
				ImGui::SliderFloat("L4", &bloomLevels[4], 0.0f, 1.0f, "%.3f", 3.0f);

				ImGui::TreePop();
			}
		}

		if (exposure != lastExposure)
		{
			GLint loc = glGetUniformLocation(finalProgram, "u_exposure");
			glProgramUniform1f(finalProgram, loc, exposure);
			lastExposure = exposure;
		}

		if (vignette != lastVignette)
		{
			GLint loc = glGetUniformLocation(finalProgram, "u_vignette_falloff");
			glProgramUniform1f(finalProgram, loc, vignette);
			lastVignette = vignette;
		}

		if (gamma != lastGamma)
		{
			GLint loc = glGetUniformLocation(finalProgram, "u_gamma");
			glProgramUniform1f(finalProgram, loc, gamma);
			lastGamma = gamma;
		}

		{
			for (int i = 0; i < bloomPass.numBlurLevels; i++)
			{
				bloomLevels[i] *= float(useBloom);
			}
			GLint loc = glGetUniformLocation(finalProgram, "u_bloom_levels[0]");
			glProgramUniform1fv(finalProgram, loc, bloomPass.numBlurLevels, bloomLevels);
		}
	}
	
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(finalProgram);
	{
		glBindTextureUnit(0, lightBuffer.lightTexture);
		glBindTextureUnit(1, bloomPass.bloomResults);

		glBindVertexArray(emptyVertexArray);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}

	glEnable(GL_DEPTH_TEST);
}

void FinalPass::ProgramLoaded(GLuint program)
{
	finalProgram = program;
	glProgramUniform1i(finalProgram, PredefinedUniformLocation(u_texture), 0);
	glProgramUniform1i(finalProgram, glGetUniformLocation(finalProgram, "u_bloom_texture"), 1);
}


