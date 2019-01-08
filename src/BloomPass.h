#pragma once

#include <glad/glad.h>

#include <vector>

#include "ShaderDependant.h"
#include "LightBuffer.h"

class BloomPass : ShaderDepandant
{
public:

	void Draw(const LightBuffer& lightBuffer);
	void ProgramLoaded(GLuint program) override;

	float threshold = 22.0f;

	const int blurBaseSize = 1024;
	const int numBlurLevels = 5;

	// alias for bloomTextures[1]
	GLuint bloomResults;

private:

	void Setup();

	GLuint GetHighPassedLightBuffer(const LightBuffer& lightBuffer, float threshold);

	GLuint emptyVertexArray{ 0 };

	GLuint highPassedTexture{};

	GLuint *highPassProgram{};
	GLint highPassTresholdLoc;

	GLuint bloomTextures[2];
	std::vector<GLuint> framebuffers;

	GLuint *blurVerticalProgram{ 0 };
	GLint blurVtextureSizeLoc{};
	GLint blurVtextureLodLoc{};

	GLuint *blurHorizontalProgram{ 0 };
	GLint blurHtextureSizeLoc{};
	GLint blurHtextureLodLoc{};

};
