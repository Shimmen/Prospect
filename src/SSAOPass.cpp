#include "SSAOPass.h"

#include <array>
#include <random>

#include <glm/glm.hpp>
using namespace glm;

#include <imgui.h>

#include "GuiSystem.h"
#include "ShaderSystem.h"
#include "TextureSystem.h"

#include "shader_locations.h"
#include "ssao_data.h"

void
SSAOPass::Draw(const GBuffer& gBuffer)
{
	static int lastW = 0;
	static int lastH = 0;
	if (lastW != gBuffer.width || lastH != gBuffer.height)
	{
		glDeleteTextures(1, &occlusionTexture);
		occlusionTexture = TextureSystem::CreateTexture(gBuffer.width, gBuffer.height, GL_R16F, GL_NEAREST, GL_NEAREST);
		
		// Setup the swizzle for the occlusion texture so it's gray scale
		GLenum swizzle[] = { GL_RED, GL_RED, GL_RED, GL_ALPHA };
		glTextureParameteriv(occlusionTexture, GL_TEXTURE_SWIZZLE_RGBA, (GLint *)swizzle);

		lastW = gBuffer.width;
		lastH = gBuffer.height;
	}

	if (!ssaoProgram)
	{
		ShaderSystem::AddComputeProgram(&ssaoProgram, "post/ssao.comp.glsl", this);
		ShaderSystem::AddComputeProgram(&ssaoBlurProgram, "post/ssao_blur.comp.glsl");

		glCreateBuffers(1, &ssaoDataBuffer);
		glNamedBufferStorage(ssaoDataBuffer, sizeof(SSAOData), nullptr, GL_DYNAMIC_STORAGE_BIT);
		glBindBufferBase(GL_UNIFORM_BUFFER, PredefinedUniformBlockBinding(SSAODataBlock), ssaoDataBuffer);

		GenerateAndUpdateKernel();
		kernelNoiseTexture = TextureSystem::LoadDataTexture("assets/blue_noise/64/LDR_LLL1_0.png", GL_R8);
	}

	if (ImGui::CollapsingHeader("SSAO"))
	{
		if (ImGui::Button("Generate new kernel"))
		{
			GenerateAndUpdateKernel();
		}

		ImGui::SliderFloat("Kernel radius", &kernelRadius, 0.01f, 10.0f);
		ImGui::SliderFloat("Intensity", &intensity, 0.0f, 20.0f);
		
		ImGui::Text("Occlusion:");
		GuiSystem::Texture(occlusionTexture);
	}

	if (kernelRadius != ssaoData.kernel_radius)
	{
		glNamedBufferSubData(ssaoDataBuffer, offsetof(SSAOData, kernel_radius), sizeof(SSAOData::kernel_radius), &kernelRadius);
	}
	if (intensity != ssaoData.intensity)
	{
		glNamedBufferSubData(ssaoDataBuffer, offsetof(SSAOData, intensity), sizeof(SSAOData::intensity), &intensity);
	}

	int xGroups = int(ceil(gBuffer.width / 32.0f));
	int yGroups = int(ceil(gBuffer.height / 32.0f));

	// Generate SSAO

	glUseProgram(*ssaoProgram);

	glBindTextureUnit(0, gBuffer.normalTexture);
	glBindTextureUnit(1, gBuffer.depthTexture);

	glBindImageTexture(0, occlusionTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
	glBindImageTexture(1, kernelNoiseTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
	
	glDispatchCompute(xGroups, yGroups, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// Blur SSAO texture

	glUseProgram(*ssaoBlurProgram);
	glBindImageTexture(0, occlusionTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R16F);
	glDispatchCompute(xGroups, yGroups, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

}

void
SSAOPass::ProgramLoaded(GLuint program)
{
	if (ssaoProgram && program == *ssaoProgram)
	{
		glProgramUniform1i(*ssaoProgram, PredefinedUniformLocation(u_g_buffer_normal), 0);
		glProgramUniform1i(*ssaoProgram, PredefinedUniformLocation(u_g_buffer_depth), 1);
	}
}

void
SSAOPass::GenerateAndUpdateKernel() const
{
	std::array<vec4, SSAO_KERNEL_SAMPLE_COUNT> kernel{};

	std::random_device device;
	std::default_random_engine rng{ device() };
	std::uniform_real_distribution<float> randomFloat{ 0.0f, 1.0f };

	for (int i = 0; i < SSAO_KERNEL_SAMPLE_COUNT; i++)
	{
		kernel[i].x = randomFloat(rng) * 2.0f - 1.0f;
		kernel[i].y = randomFloat(rng) * 2.0f - 1.0f;
		kernel[i].z = randomFloat(rng);
		kernel[i].w = 0.0f; // (unused)

		float scale = float(i) / float(SSAO_KERNEL_SAMPLE_COUNT);
		scale = mix(0.1f, 1.0f, scale * scale);

		kernel[i] *= (1.0f / length(kernel[i])) * scale;
	}

	glNamedBufferSubData(ssaoDataBuffer, offsetof(SSAOData, kernel), sizeof(SSAOData::kernel), kernel.data());
}
