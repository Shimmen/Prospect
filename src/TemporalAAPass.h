#pragma once

#include <glad/glad.h>

#include "GBuffer.h"
#include "LightBuffer.h"
#include "UniformValue.h"
#include "ShaderDependant.h"


class TemporalAAPass : ShaderDepandant
{
public:

	void Draw(const LightBuffer& lightBuffer, const GBuffer& gBuffer);
	void ProgramLoaded(GLuint program) override;

	Uniform<float> historyBlend{ "u_history_blend", 0.05f };

	bool enabled;
	GLuint outputTexture;

private:

	GLuint *taaProgram;
	GLuint historyTexture;

	int frameCount = 0;
	bool ShouldSetFirstFrame(int width, int height, int frameCount) const;

	GLint firstFrameLocation;

};
