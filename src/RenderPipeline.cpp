#include "RenderPipeline.h"

#include <glm/glm.hpp>

#include "TextureSystem.h"
#include "PerformOnce.h"
#include "Input.h"
#include "Scene.h"

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
		blueNoiseTexture = TextureSystem::LoadDataTexture("assets/blue_noise/64/LDR_LLL1_0.png", GL_R8); // TODO: Use texture array!
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

	scene.mainCamera->CommitToGpu(deltaTime);

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
