#pragma once

#include "GBuffer.h"
#include "LightBuffer.h"
#include "SSAOPass.h"
#include "SkyPass.h"
#include "IBLPass.h"
#include "GeometryPass.h"
#include "FinalPass.h"
#include "ShadowMap.h"
#include "LightPass.h"
#include "ShadowPass.h"

class Input;
struct Scene;

class RenderPipeline
{
public:

	struct
	{
		bool useTaa = true;
		// TODO: Add more settings here!

	} settings;

	void Resize(int width, int height);
	void Render(Scene& scene, const Input& input, float deltaTime, float runningTime);

private:

	int width;
	int height;

	GLuint blueNoiseTexture;

	GBuffer gBuffer{};
	LightBuffer lightBuffer{};
	ShadowMap shadowMapAtlas{};

	GeometryPass geometryPass{};
	ShadowPass shadowPass{};
	LightPass lightPass{};

	IBLPass iblPass{};
	SkyPass skyPass{};

	SSAOPass ssaoPass{};
	FinalPass finalPass{};

	bool resizeThisFrame = false;
	unsigned int frameCount = 0;

};
