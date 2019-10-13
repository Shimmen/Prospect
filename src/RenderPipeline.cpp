#include "RenderPipeline.h"

#include <glm/glm.hpp>

#include "TextureSystem.h"
#include "PerformOnce.h"
#include "Input.h"
#include "Scene.h"

#include "shader_locations.h"

void RenderPipeline::Resize(int width, int height)
{
	this->width = width;
	this->height = height;

	gBuffer.RecreateGpuResources(width, height);
	lightBuffer.RecreateGpuResources(width, height, gBuffer);

	resizeThisFrame = true;
}

void RenderPipeline::Render(Scene& scene, const Input& input, float deltaTime, float runningTime)
{
	PerformOnce(
		shadowMapAtlas.RecreateGpuResources(8192);
		sceneBuffer.BindBufferBase(BufferObjectType::Uniform, PredefinedUniformBlockBinding(SceneUniformBlock));
		
		blueNoiseTexture = TextureSystem::LoadBlueNoiseTextureArray("assets/blue_noise/64/");
		glBindImageTexture(PredefinedImageBinding(BlueNoiseImage), blueNoiseTexture, 0, GL_FALSE, 0, GL_READ_ONLY, GL_R8);
	);

	if (settings.useTaa)
	{
		// Generate jitter samples in pixel space, centered around 0, with offsets -0.5 to +0.5
		glm::vec2 haltonSample = Halton(frameCount, 2, 3);
		glm::vec2 offsetPixels = haltonSample - 0.5f;
		scene.mainCamera->ApplyFrustumJitter(offsetPixels);
	}
	else
	{
		scene.mainCamera->ApplyFrustumJitter({ 0, 0 });
	}

	sceneBuffer.memory.delta_time = deltaTime;
	sceneBuffer.memory.running_time = runningTime;
	sceneBuffer.memory.frame_count = frameCount;
	sceneBuffer.memory.frame_count_noise = settings.useTaa ? frameCount % 64 : 0;
	sceneBuffer.UpdateGpuBuffer();

	scene.mainCamera->CommitToGpu();

	geometryPass.Draw(gBuffer, scene);
	shadowPass.Draw(shadowMapAtlas, scene);

	ssaoPass.Draw(gBuffer);

	iblPass.Draw(lightBuffer, gBuffer, ssaoPass, scene);
	lightPass.Draw(lightBuffer, gBuffer, shadowMapAtlas, scene);
	skyPass.Draw(lightBuffer, gBuffer, scene);

	gBuffer.RenderGui("before final");
	finalPass.Draw(gBuffer, lightBuffer, scene, &settings.useTaa);

	frameCount += 1;
	resizeThisFrame = false;
}
