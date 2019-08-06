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
		if (randomKernelSamples && ImGui::Button("Generate new kernel"))
		{
			GenerateAndUpdateKernel();
		}

		ImGui::SliderFloat("Kernel radius", &kernelRadius, 0.01f, 3.0f);
		ImGui::SliderFloat("Intensity", &intensity, 0.0f, 20.0f);

		ImGui::Checkbox("Apply blur", &applyBlur);

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

	glBindTextureUnit(0, gBuffer.normVelTexture);
	glBindTextureUnit(1, gBuffer.depthTexture);

	glBindImageTexture(0, occlusionTexture, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_R16F);
	glBindImageTexture(1, kernelNoiseTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);

	glDispatchCompute(xGroups, yGroups, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	// Blur SSAO texture

	if (applyBlur)
	{
		glUseProgram(*ssaoBlurProgram);
		glBindImageTexture(0, occlusionTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R16F);
		glDispatchCompute(xGroups, yGroups, 1);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}
}

void
SSAOPass::ProgramLoaded(GLuint program)
{
	if (ssaoProgram && program == *ssaoProgram)
	{
		glProgramUniform1i(*ssaoProgram, PredefinedUniformLocation(u_g_buffer_norm_vel), 0);
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
		kernel[i].w = 0.0f; // (unused)

		if (randomKernelSamples)
		{
			float xi = randomFloat(rng);
			float radius = sqrtf(xi);

			float theta = randomFloat(rng) * 2.0f * 3.141593f;
			kernel[i].x = radius * cosf(theta);
			kernel[i].y = radius * cosf(theta);
			kernel[i].z = 1.0 - xi;
		}
		else
		{
			// A spherical Fibonacci lattice sampling strategy. Seems to work pretty fine...
			// http://extremelearning.com.au/evenly-distributing-points-on-a-sphere/

			const float phi = (1.0f + sqrtf(5.0f)) / 2.0f;

			float x1 = float(i) / float(SSAO_KERNEL_SAMPLE_COUNT - 1);
			float x2 = float(i) / phi;

			float radius = sqrtf(x1);
			float angle = 2.0f * 3.141593f * x2;

			kernel[i].x = radius * cosf(angle);
			kernel[i].y = radius * sinf(angle);

			// Project z-component so the sample lies on the unit sphere.
			// Also avoid samples too close to the surface / bottom of the kernel
			kernel[i].z = 0.75f * cosf(radius) + 0.25f;
		}

		// Scale the samples to the kernel fills its own space
		float scale = float(i) / float(SSAO_KERNEL_SAMPLE_COUNT - 1);
		scale = mix(0.1f, 1.0f, scale * scale);
		kernel[i] *= scale;
	}

	glNamedBufferSubData(ssaoDataBuffer, offsetof(SSAOData, kernel), sizeof(SSAOData::kernel), kernel.data());
}
