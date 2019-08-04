#pragma once

#include "GBuffer.h"
#include "LightBuffer.h"
#include "SSAOPass.h"
#include "SkyPass.h"
#include "IBLPass.h"
#include "GeometryPass.h"
#include "FinalPass.h"

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

	GBuffer gBuffer{};
	LightBuffer lightBuffer{};

	GeometryPass geometryPass{};
	IBLPass iblPass{};
	SkyPass skyPass{};
	SSAOPass ssaoPass{};
	FinalPass finalPass{};

	bool resizeThisFrame = false;
	unsigned int frameCount = 0;

};
