#include "IBLPass.h"

#include <random>

#include <imgui.h>

#include "Scene.h"
#include "GuiSystem.h"
#include "ModelSystem.h"
#include "TextureSystem.h"
#include "FullscreenQuad.h"

using namespace glm;
#include "shader_locations.h"
#include "shader_constants.h"
#include "shader_types.h"

void
IBLPass::Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer, const SSAOPass& ssaoPass, Scene& scene)
{
	if (!iblProgram)
	{
		ShaderSystem::AddProgram("light/ibl", this);
	}

	if (!brdfIntegrationMap) { CreateBrdfIntegrationMap(); }
	if (!scene.skyProbe.diffuseIrradianceSh) { scene.skyProbe.diffuseIrradianceSh = TextureSystem::CreatePlaceholder(0xDD, 0xDD, 0xDD); }
	if (!scene.skyProbe.filteredRadiance) { scene.skyProbe.filteredRadiance = TextureSystem::CreatePlaceholder(0xFF, 0xFF, 0xFF); }

	static bool filteringPerformed = false;
	if (!filteringPerformed && TextureSystem::IsIdle() && ModelSystem::IsIdle())
	{
		FilterProbe(scene.skyProbe);
		filteringPerformed = true;
	}

	if (ImGui::CollapsingHeader("Image Based Lighting (IBL)"))
	{
		if (ImGui::Button("Perform filtering now!"))
		{
			FilterProbe(scene.skyProbe);
		}

		ImGui::Text("Filtered radiance");
		GuiSystem::Texture(scene.skyProbe.filteredRadiance, 2.0f);

		static bool overrideRoughness = false;
		bool toggled = ImGui::Checkbox("Override min roughness", &overrideRoughness);

		if (overrideRoughness)
		{
			static float roughness = 0.0f;
			ImGui::SliderFloat("Manual min roughness", &roughness, 0.0f, 1.0f);

			float minLod = roughness * float(IBL_RADIANCE_MIPMAP_LAYERS - 1);
			glTextureParameterf(scene.skyProbe.filteredRadiance, GL_TEXTURE_MIN_LOD, minLod);
		}
		else if (toggled)
		{
			glTextureParameterf(scene.skyProbe.filteredRadiance, GL_TEXTURE_MIN_LOD, -1'000.0f);
		}

		ImGui::Text("BRDF integration map");
		GuiSystem::Texture(brdfIntegrationMap, 1.0f);
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, lightBuffer.framebuffer);
	glViewport(0, 0, lightBuffer.width, lightBuffer.height);

	glUseProgram(iblProgram);
	glDisable(GL_DEPTH_TEST);

	glBindTextureUnit(0, gBuffer.albedoTexture);
	glBindTextureUnit(1, gBuffer.materialTexture);
	glBindTextureUnit(2, gBuffer.normalTexture);
	glBindTextureUnit(3, gBuffer.depthTexture);

	glBindTextureUnit(5, scene.skyProbe.diffuseIrradianceSh);
	glBindTextureUnit(6, scene.skyProbe.filteredRadiance);
	glBindTextureUnit(7, brdfIntegrationMap);

	glBindTextureUnit(8, ssaoPass.occlusionTexture);

	FullscreenQuad::Draw();

	glEnable(GL_DEPTH_TEST);
}

void
IBLPass::ProgramLoaded(GLuint program)
{
	iblProgram = program;

	glProgramUniform1i(iblProgram, PredefinedUniformLocation(u_g_buffer_albedo), 0);
	glProgramUniform1i(iblProgram, PredefinedUniformLocation(u_g_buffer_material), 1);
	glProgramUniform1i(iblProgram, PredefinedUniformLocation(u_g_buffer_normal), 2);
	glProgramUniform1i(iblProgram, PredefinedUniformLocation(u_g_buffer_depth), 3);

	GLint locIrradianceSh = glGetUniformLocation(iblProgram, "u_irradiance_sh");
	glProgramUniform1i(iblProgram, locIrradianceSh, 5);

	GLint locRadiance = glGetUniformLocation(iblProgram, "u_radiance");
	glProgramUniform1i(iblProgram, locRadiance, 6);

	GLint locBrdf = glGetUniformLocation(iblProgram, "u_brdf_integration_map");
	glProgramUniform1i(iblProgram, locBrdf, 7);

	GLint locOcclusion = glGetUniformLocation(iblProgram, "u_occlusion_texture");
	glProgramUniform1i(iblProgram, locOcclusion, 8);
}

void
IBLPass::CreateBrdfIntegrationMap()
{
	const int size = 512;

	GLuint map;
	glCreateTextures(GL_TEXTURE_2D, 1, &map);
	glTextureStorage2D(map, 1, GL_RG16F, size, size);

	glTextureParameteri(map, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(map, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(map, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(map, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	GLuint unit = 0;
	glBindImageTexture(unit, map, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RG16F);

	GLuint program = *ShaderSystem::AddComputeProgram("ibl/brdf_map.comp.glsl");

	glUseProgram(program);
	glDispatchCompute(size, size, 1);

	// NOTE: This might not be required here, since we don't need the result immediatly
	//glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	brdfIntegrationMap = map;
}

void
IBLPass::FilterProbe(Probe& probe)
{
	const GLenum format = GL_RGBA16F;

	// First, make sure the radiance map is square and generate mipmaps for it!
	// (for now we just assume that radianceMap is not square)

	GLuint squareMap;
	glCreateTextures(GL_TEXTURE_2D, 1, &squareMap);

	int numMipmaps = static_cast<int>(std::log2(IBL_RADIANCE_BASE_SIZE));
	glTextureStorage2D(squareMap, numMipmaps, format, IBL_RADIANCE_BASE_SIZE, IBL_RADIANCE_BASE_SIZE);

	glTextureParameteri(squareMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(squareMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(squareMap, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(squareMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	static GLuint *resizeProgram = ShaderSystem::AddComputeProgram("etc/resize.comp.glsl");

	glProgramUniform1i(*resizeProgram, glGetUniformLocation(*resizeProgram, "u_source"), 0);
	glBindTextureUnit(0, probe.radiance);

	glBindImageTexture(0, squareMap, 0, GL_FALSE, 0, GL_WRITE_ONLY, format);

	glUseProgram(*resizeProgram);
	glDispatchCompute(IBL_RADIANCE_BASE_SIZE / 32, IBL_RADIANCE_BASE_SIZE / 32, 1);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	glGenerateTextureMipmap(squareMap);

	// Filtered radiance
	{
		const int numLevels = IBL_RADIANCE_MIPMAP_LAYERS;

		GLuint filteredMap;
		glCreateTextures(GL_TEXTURE_2D, 1, &filteredMap);
		glTextureStorage2D(filteredMap, numLevels, format, IBL_RADIANCE_BASE_SIZE, IBL_RADIANCE_BASE_SIZE);

		glTextureParameteri(filteredMap, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(filteredMap, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(filteredMap, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(filteredMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		static GLuint *program = ShaderSystem::AddComputeProgram("ibl/filter_radiance.comp.glsl");
		glUseProgram(*program);

		glProgramUniform1i(*program, glGetUniformLocation(*program, "u_radiance"), 0);
		glBindTextureUnit(0, squareMap);

		GLint roughnessLocation = glGetUniformLocation(*program, "u_roughness");

		for (int level = 0; level < numLevels; ++level)
		{
			float roughness = float(level) / float(numLevels - 1);

			glProgramUniform1f(*program, roughnessLocation, roughness);
			glBindImageTexture(0, filteredMap, level, GL_FALSE, 0, GL_WRITE_ONLY, format);

			int textureSize = IBL_RADIANCE_BASE_SIZE / int(pow(2, level));
			int localSize = IBL_RADIANCE_OPT_LOCAL_SIZE;
			glDispatchCompute(textureSize / localSize, textureSize / localSize, 1);
		}

		probe.filteredRadiance = filteredMap;
	}

	// Diffuse irradiance spherical harmonics
	// TODO: Store in some better way than a texture?
	{
		GLuint shMap;
		glCreateTextures(GL_TEXTURE_2D, 1, &shMap);
		glTextureStorage2D(shMap, 1, GL_RGBA16F, 3, 3);

		glTextureParameteri(shMap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTextureParameteri(shMap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTextureParameteri(shMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(shMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		static GLuint *program = ShaderSystem::AddComputeProgram("ibl/create_sh.comp.glsl");
		glUseProgram(*program);

		if (!sphereSampleBuffer)
		{
			std::default_random_engine rng{};
			std::uniform_real_distribution<float> randomFloat{ -1.0f, 1.0f };

			static float sampleData[SPHERE_SAMPLES_COUNT * 4];
			for (int i = 0; i < SPHERE_SAMPLES_COUNT; ++i)
			{
				float x, y, z;
				do
				{
					x = randomFloat(rng);
					y = randomFloat(rng);
					z = randomFloat(rng);
				} while ((x * x + y * y + z * z) <= 1.0);

				sampleData[4 * i + 0] = x;
				sampleData[4 * i + 1] = y;
				sampleData[4 * i + 2] = z;
				// alpha for padding, so unusued!
			}

			glCreateBuffers(1, &sphereSampleBuffer);
			glNamedBufferStorage(sphereSampleBuffer, sizeof(sampleData), sampleData, GL_DYNAMIC_STORAGE_BIT);
			glBindBufferBase(GL_UNIFORM_BUFFER, PredefinedUniformBlockBinding(SphereSampleBuffer), sphereSampleBuffer);
		}

		glProgramUniform1i(*program, glGetUniformLocation(*program, "u_radiance"), 0);
		glBindTextureUnit(0, squareMap);

		glBindImageTexture(0, shMap, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA16F);

		glDispatchCompute(1, 1, 1);

		probe.diffuseIrradianceSh = shMap;
	}
}
